#!/bin/sh
# Packs the build into dist/cataclysm2-<target>.tar.gz for av: the binary
# at the top, the read-only game data beside it in data/ and cuss/ (where
# the game looks first), the licence and the readmes. Worlds are saved in
# the player's data folder, never in the install folder.
#
# Left out: data/termfont and data/FONTDATA (a Terminus bitmap font and its
# settings, used only by upstream's own Windows window, not by a terminal),
# cuss/editor (the interface editor's own screens) and data_crazy/ (an
# unused alternative data set).
#
#   asciiverse/package.sh x86_64-unknown-linux-musl
set -eu
cd "$(dirname "$0")/.."
target="$1"
stage=stage/cataclysm2
rm -rf "$stage" && mkdir -p "$stage" dist
cp out/cataclysm2 LICENSE "$stage/"
cp README.md "$stage/README.upstream.md"
cp asciiverse/README.txt "$stage/README.txt"
cp -r data "$stage/data"
rm -f "$stage/data/termfont" "$stage/data/FONTDATA"
mkdir -p "$stage/cuss"
cp cuss/*.cuss "$stage/cuss/"
tar -czf "dist/cataclysm2-$target.tar.gz" -C "$stage" .
ls dist
