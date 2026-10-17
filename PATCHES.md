# Changes from upstream

This is the Asciiverse packaging of [Cataclysm II](https://github.com/Whales/Cataclysm2)
by Whales, with contributions from Aninimouse, Trevmillion, superninjakiwi,
WeendVidente, Drakasin and others (GPL-3.0, `LICENSE`; `data/motd.txt`
says GPLv3 too). It is Whales' unfinished ground-up rewrite of Cataclysm,
an early alpha, abandoned in 2016. It is packaged as it is: playable for a
while, with much missing, and characters cannot be saved. Nothing here
tries to finish it.

Upstream is the `upstream` remote. The `asciiverse` branch starts from
upstream's `master` at `8635e2ec3f714a7b6549b70cfadd36e38ef8c6b1`
(2016-07-01, "Updating map.cpp", tag `whales-8635e2e`), its full history of
1,430 commits. GitHub's archive of that commit,
`Cataclysm2-8635e2e.tar.gz`, had sha256
`5e5dc894416f8693dde1ba034f3c3f29ff006cc1bf92643deceb1bf5a7d72a66` on
2026-10-17 (GitHub does not promise these archives stay byte-identical;
the commit id is the reference). No fork of it is maintained: of 31 forks,
two have commits of their own (Wad67, 2017, one UI fix-up; mytinylab,
2025, a Windows build), neither taken.

## Known patches (one commit each, their own authors)

- Pull request #55 by keyspace (2017-01-31, never merged; tag `pr55`,
  `pr55.patch` sha256
  `246c4a27bf84bd0601e048d27daf5dd27ede536b5881a132340b08676a6960e9`):
  upstream's last commit was half a change and did not compile; these four
  commits make it build (`const` on two `Sight_map` methods, the
  unfinished `build_tile_sight_map()` closed and declared, a `may_y`
  typo). Their author field is the placeholder `Your Name
  <user@example.com>` from the pull request; each commit message names
  the pull request.
- Pull request #45 by incorpse (2014-04-24, never merged; tag `pr45`,
  `pr45.patch` sha256
  `188cbca03f1a22c91e9f66e342962928f5320198e5d9b3414a623334de2f9f27`):
  the A* pathfinder's array of `Tripoint` with run-time sizes, which clang
  (macOS) refuses, becomes nested vectors. Its third commit, a new
  `time.h` header guard, is left out: upstream made the same change since.

## Bugs fixed (found by running it under AddressSanitizer and UBSan)

- Text wrapping read one past the end of the text (the first screen).
- Every window line was formatted with `vsprintf()` into a fixed buffer
  and then passed to `printw()` as a format a second time: bounded
  `vsnprintf()` and `addstr()` now. Blanked cells were a black `x` on
  black; they are spaces.
- The interface files hold uninitialised vertical alignments saved by the
  editor (156362380 and the like); out-of-range values read as top, which
  is how they were drawn, and elements made in code start aligned.
- Thrown items were cast to a launcher type to read a skill from past the
  end of their item type.
- The skills screen read (and could write) past the 24-entry skill table
  for letters without a skill.
- `list_items()` ("You drop a knife, a hatchet and ...") compared against
  the wrong index while sorting and read past the end of its list.
- A dead monster was deleted before it was erased from the entity pool,
  which reads its uid: a use after free on every death.
- Mapgen checked a chance that is never read for `all_items:` and
  `all_item_group:`; uninitialised, it stopped optimised builds on the
  starting beach with a debug message.
- From the compiler's warnings: melee damage left uninitialised one hit in
  three (and pierce damage rolled against the cut adjustment), ranged
  damage multipliers unset for some hits, an uninitialised item pointer
  when applying an item not carried, `!class == BOOK` accepting any item
  as a book, an unset night light level, and `closedir(NULL)` when a
  folder could not be opened.

## Behaviour

- Folders: `data/` and `cuss/` are looked for beside the program first
  (`/proc/self/exe`, `_NSGetExecutablePath`, `GetModuleFileName`), then in
  the current folder. Worlds are saved in `$XDG_DATA_HOME/cataclysm2`,
  else `~/.local/share/cataclysm2` (`%APPDATA%\cataclysm2` on Windows),
  made on the first run, instead of `./save`. The mapgen files follow the
  data folder (they were read from a fixed `data/mapgen`). `--data-dir`,
  `--cuss-dir` and `--save-dir` still override.
- L: Load Character was never written and quit the program silently; it
  says so in a popup and stays in the menu.
- `<` and `>` moved the player up or down anywhere (a debugging shortcut),
  into the ground; that is `--test` only now.
- Esc answers in 25 ms instead of ncurses' one second (`set_escdelay`).

## Packaging (new files)

- `asciiverse/build.sh` builds the game with upstream's Makefile at
  `-j2` (not the interface editor or its test program);
  `asciiverse/package.sh` packs `dist/cataclysm2-<target>.tar.gz`: the
  binary, `data/` (without `termfont` and `FONTDATA`, the Terminus bitmap
  font for upstream's own Windows window, which has no licence notice and
  is not used in a terminal), `cuss/*.cuss` (not the editor's screens),
  `LICENSE`, upstream's `README.md` as `README.upstream.md` and
  `asciiverse/README.txt`. `data_crazy/` and `docs/` are not shipped.
- `.github/workflows/release.yml`: static musl Linux in Alpine (x86_64,
  aarch64) and macOS (x86_64, arm64). No Windows build: upstream's Windows
  port draws in a GDI window of its own (`catacurse.cpp`), not a terminal.
- `asciiverse/record/`: the preview driver (`cataclysm2.py`, with
  `rec.py` from the NetBSD games) and `asciiverse/casts/cataclysm2.cast`.
- `.gitignore`: the build folders.
