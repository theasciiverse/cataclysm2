#!/bin/sh
# Builds cataclysm2 into out/ with upstream's Makefile. The compiler, flags
# and curses library come from the environment, so one script serves every
# target (see .github/workflows/release.yml):
#
#   asciiverse/build.sh                                         # Debian, Ubuntu
#   LIBS=-lncursesw LDFLAGS="-static -s" asciiverse/build.sh    # Alpine, static
#   CXXFLAGS="-O2 -arch arm64" LDFLAGS="-arch arm64" asciiverse/build.sh  # macOS
#
# Only the game is built; upstream's interface editor (cussed) and its test
# program are not part of the catalog title.
set -eu
cd "$(dirname "$0")/.."
CXX="${CXX:-c++}"
CXXFLAGS="${CXXFLAGS:--O2}"
LDFLAGS="${LDFLAGS:-}"
LIBS="${LIBS:--lncurses}"
rm -rf obj cataclysm2
mkdir -p out
make -j2 cataclysm2 \
  CXX="$CXX" WARNINGS= DEBUG= OTHERS="$CXXFLAGS" LDFLAGS="$LDFLAGS $LIBS"
mv cataclysm2 out/cataclysm2
echo "built cataclysm2"
