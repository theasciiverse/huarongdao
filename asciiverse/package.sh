#!/bin/sh
# Packs the build into dist/huarongdao-<target>.tar.gz for av: the hrd
# binary at the top, with the man page, COPYING, README and PATCHES.md.
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/huarongdao
rm -rf "$stage" && mkdir -p "$stage" dist
cp out/hrd out/hrd.6 COPYING README PATCHES.md "$stage/"
tar -czf "dist/huarongdao-$target.tar.gz" -C "$stage" .
ls dist
