#!/bin/sh
# Builds hrd (HuaRongDao) into out/ without autotools: the six sources in
# one compiler run (one process, so no parallel jobs to limit), with
# asciiverse/config.h in place of the config.h configure writes. The
# compiler, flags and curses library come from the environment, so one
# script serves every target (see .github/workflows/release.yml):
#
#   asciiverse/build.sh                                        # Linux
#   LDFLAGS="-static -s" asciiverse/build.sh                   # Alpine, static
#   LIBS=-lncurses CXXFLAGS="-O2 -arch arm64" \
#     LDFLAGS="-arch arm64" asciiverse/build.sh                # macOS
set -eu
cd "$(dirname "$0")/.."
CXX="${CXX:-c++}"
CXXFLAGS="${CXXFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
LIBS="${LIBS:--lncursesw}"
mkdir -p out
rm -f out/hrd
# shellcheck disable=SC2086
nice -n 10 $CXX $CXXFLAGS -DHAVE_CONFIG_H -Iasciiverse -o out/hrd \
  hrd.cc board.cc iface.cc ncurses.cc game.cc bookmark.cc $LDFLAGS $LIBS
cp hrd.6 out/hrd.6
echo "built hrd"
