# Changes from upstream

This is the Asciiverse packaging of [HuaRongDao](https://sourceforge.net/projects/hrd/)
(`hrd`), Qingning Huo's curses version of the Chinese sliding-block
puzzle, GPL-2.0-or-later (README and the source headers: "either version
2 of the License, or (at your option) any later version"; COPYING is the
GPL 2). There is no public repository, so this one starts from the only
release:

- `hrd-0.1.tar.gz` from SourceForge (hrd/0.1, 2004-04-25), retrieved
  2026-10-18, sha256
  `950a52f80c008894f29027d4f22c5c979d27f5510f50580dacff71e1456dfc0d`,
  imported unchanged as the first commit (credited to Qingning Huo) and
  tagged `0.1`.

The game ships one layout, the classic one (`TQQT.BQQB.TLRT.BPPB.P__P.`).
Others can be played by giving a pattern on the command line (see the
man page); none are added here.

## Debian

Debian packaged hrd 0.1-1 to 0.1-3 (2004 to 2005, maintained by Qingning
Huo himself; last in squeeze). The changes were made in the source
directly, without patch files, and no packaging repository is named.
Its diff against the tarball, taken from sources.debian.org, is two
commits with his name and the Debian dates, tagged `debian/0.1-3`:

- "Fix build error on amd64/gcc-4.0" (0.1-2, Closes: #303994): the inline
  `Lock::~Lock()` in `hrd.h` is dropped (it is also defined in
  `bookmark.cc`).
- Newer `config.guess` and `config.sub` (0.1-3). `debian/` is not
  included.

## Code

- **Bookmarks in the XDG data folder; no crash without HOME.** Bookmarks
  were in `~/.hrd/bookmark`, and a missing `HOME` crashed the game at
  start. They are in `$XDG_DATA_HOME/hrd/bookmark` (`~/.local/share/hrd`
  when unset, empty or relative), missing folders created; an existing
  `~/.hrd/bookmark` stays in use while the new folder does not exist;
  with no `HOME` the game runs without bookmarks. The man page follows.
- **mmap() failures detected.** Saving and loading compared `mmap()`'s
  result with 0 instead of `MAP_FAILED`; loading from the empty bookmark
  file of a first run handed `(char*)-1` on. An empty file is reported
  without mapping it.
- **Centred again after a resize; key codes kept out of `<ctype.h>`.** A
  resize reached the game as an unknown key and the board stayed put;
  it now redraws as Ctrl-L does, centred in the new size. Cursor and
  function keys (codes above 255) were passed to `isdigit()` and
  `isalnum()`; `key_isdigit()`/`key_isalnum()` check the range first.
  The locale is set before curses starts (UTF-8 boxes in tmux, screen,
  the Linux console).

## Packaging (new files)

- `asciiverse/build.sh`: no autotools; the six sources in one C++
  compiler run against wide ncurses, with `asciiverse/config.h` standing
  in for configure's `config.h`.
- `asciiverse/package.sh`: one archive per target with `hrd`, the man
  page, COPYING, README and this file.
- `.github/workflows/release.yml`: static Linux in Alpine (x86_64,
  aarch64) and macOS (x86_64, arm64), plus SHA256SUMS, on a tag. No
  Windows build: bookmarks use `mmap()` and `fcntl()` record locks.
- `asciiverse/record-preview.py`: records `asciiverse/huarongdao.cast`;
  a driver reads the pieces' numbers from the screen, finds a shortest
  solution (116 one-cell moves) by breadth-first search and plays it to
  "Solved! Well done!".
- `.gitignore`.

## Tested

Linux: native (glibc, ncursesw 6.6) and static (Alpine 3.22, musl)
builds; the classic layout solved by the recorder; bookmarks saved and
loaded, in `~/.local/share/hrd`, under `XDG_DATA_HOME`, with an old
`~/.hrd` kept, and without `HOME`; centred at 100x30 and re-centred
after a resize to 60x22. ASan and UBSan: three 40-second runs of random
keys (pieces, arrows, undo, redo, bookmarks, Ctrl-C, Ctrl-L) with random
resizes, plus custom and invalid patterns, no reports. macOS: not built
here.
