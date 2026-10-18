#!/usr/bin/env python3
"""Records asciiverse/huarongdao.cast, the catalog preview, at 100x30
(hub/RECORDING.md): the classic layout solved. The driver reads the
pieces' numbers from the screen, finds a shortest solution by a
breadth-first search over one-cell moves (the board is small: about
25,000 distinct positions), and plays it by typing each piece's number
and an arrow key, until the game says "Solved! Well done!"; any key
then ends the game.

hrd runs from out/ (asciiverse/build.sh) by name, so the cast's header
holds no local path, with a throwaway HOME, so nothing touches your own
bookmarks.

Needs asciinema 3.x and pyte. Usage, from the repo root:
  uv run --with pyte python3 asciiverse/record-preview.py
Any extra argument is used as the cast's path instead (for test runs);
with --dry-run it plays without recording."""
import collections, fcntl, os, pty, select, shutil, struct, sys, tempfile, termios, time
import pyte

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
args = [a for a in sys.argv[1:] if a != "--dry-run"]
dry = "--dry-run" in sys.argv
cast = args[0] if args else f"{root}/asciiverse/huarongdao.cast"
COLS, ROWS = 100, 30
# the default layout, as hrd.cc has it: rows of four, Q the 2x2 piece,
# T/B a vertical 1x2, L/R a horizontal 2x1, P a single square
PATTERN = "TQQT.BQQB.TLRT.BPPB.P__P."
SIZE = {"Q": (2, 2), "T": (2, 1), "L": (1, 2), "P": (1, 1)}   # (rows, columns)
ARROWS = {(-1, 0): b"\x1bOA", (1, 0): b"\x1bOB", (0, 1): b"\x1bOC", (0, -1): b"\x1bOD"}

work = tempfile.mkdtemp(prefix="hrd-preview-")
env = dict(os.environ, TERM="xterm-256color", COLORTERM="truecolor", LANG="C.UTF-8",
           HOME=work, PATH=f"{root}/out:{os.environ['PATH']}")
for v in ("VTE_VERSION", "TERM_PROGRAM", "TERM_PROGRAM_VERSION", "TMUX", "XDG_DATA_HOME"):
    env.pop(v, None)

pid, fd = pty.fork()
if pid == 0:
    os.chdir(work)
    if dry:
        os.execvpe("hrd", ["hrd"], env)
    os.execvpe("asciinema", ["asciinema", "rec", "--overwrite", "-q", "--idle-time-limit", "2",
        "--window-size", f"{COLS}x{ROWS}", "--capture-env", "TERM,COLORTERM,LANG",
        "--title", "hrd", "--command", "hrd", cast], env)
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
screen = pyte.Screen(COLS, ROWS)
stream = pyte.ByteStream(screen)
start = time.time()


def pump(seconds):
    end = time.time() + seconds
    while True:
        left = end - time.time()
        r, _, _ = select.select([fd], [], [], max(0.0, min(0.02, left)))
        if r:
            try:
                stream.feed(os.read(fd, 65536))
            except OSError:
                return False
        if left <= 0:
            return True


def key(k, wait=0.3):
    os.write(fd, k)
    pump(wait)


def pieces_from_pattern():
    """[(type, row, column)] for each piece's top left cell."""
    rows = PATTERN.strip(".").split(".")
    out = []
    for r, line in enumerate(rows):
        for c, ch in enumerate(line):
            if ch in "QTLP" and not (ch == "Q" and ((r > 0 and rows[r - 1][c] == "Q")
                                                    or (c > 0 and line[c - 1] == "Q"))):
                out.append((ch, r, c))
    return out


def numbers(pieces):
    """The number hrd shows on each piece: the first digit inside the 3x3
    block of the piece's top left cell; the board's box starts at the
    first box corner on the screen (pyte keeps ncurses' VT100 line set
    as letters, 'l' for the corner)."""
    for y, line in enumerate(screen.display):
        for corner in ("lqqqqqqqqqqqqk", "┌────────────┐"):
            x = line.find(corner)
            if x >= 0:
                out = []
                for t, r, c in pieces:
                    block = "".join(screen.display[y + 1 + 3 * r + i][x + 1 + 3 * c:x + 4 + 3 * c]
                                    for i in range(3))
                    out.append(next(ch for ch in block if ch.isdigit()))
                return out
    return None


def cells(t, r, c):
    h, w = SIZE[t]
    return [(r + i, c + j) for i in range(h) for j in range(w)]


def solve(pieces):
    """Shortest list of (piece index, (dr, dc)) one-cell moves that brings
    the 2x2 piece to the exit (rows 3-4, columns 1-2)."""
    def key_of(ps):
        grid = [["_"] * 4 for _ in range(5)]
        for t, r, c in ps:
            for rr, cc in cells(t, r, c):
                grid[rr][cc] = t
        return "".join("".join(g) for g in grid)
    start_ps = tuple(pieces)
    seen = {key_of(start_ps): None}
    queue = collections.deque([start_ps])
    while queue:
        ps = queue.popleft()
        q = next(p for p in ps if p[0] == "Q")
        if (q[1], q[2]) == (3, 1):
            path, k = [], key_of(ps)
            while seen[k] is not None:
                k, move = seen[k]
                path.append(move)
            return path[::-1]
        taken = {cell for p in ps for cell in cells(*p)}
        for i, (t, r, c) in enumerate(ps):
            own = set(cells(t, r, c))
            for dr, dc in ARROWS:
                moved = [(rr + dr, cc + dc) for rr, cc in own]
                if all(0 <= rr < 5 and 0 <= cc < 4 and ((rr, cc) in own or (rr, cc) not in taken)
                       for rr, cc in moved):
                    nps = list(ps)
                    nps[i] = (t, r + dr, c + dc)
                    nps = tuple(nps)
                    k = key_of(nps)
                    if k not in seen:
                        seen[k] = (key_of(ps), (i, (dr, dc)))
                        queue.append(nps)
    return None


pump(2.0)
pieces = pieces_from_pattern()
labels = numbers(pieces)
moves = solve(pieces)
for i, d in moves:
    key(labels[i].encode(), 0.12)   # select the piece
    key(ARROWS[d], 0.25)            # and move it one cell
    if "possible" in screen.display[ROWS - 1]:
        key(b"\r", 0.05)    # a second direction is offered; this move is done
solved = any("Solved" in line for line in screen.display)
pump(3.0)
key(b"\r", 0.5)            # any key ends the game after "Solved!"
end = time.time() + 5
while time.time() < end:
    if not pump(0.1):
        break
    done, _ = os.waitpid(pid, os.WNOHANG)
    if done:
        break
else:
    os.kill(pid, 9)
try:
    os.waitpid(pid, 0)
except ChildProcessError:
    pass
shutil.rmtree(work, ignore_errors=True)
print(f"{cast}: {time.time() - start:.0f} s, {len(moves)} one-cell moves, solved: {solved}", file=sys.stderr)
