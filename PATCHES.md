# Changes from upstream

This is the Asciiverse packaging of [Cataclysm II](https://github.com/Whales/Cataclysm2)
by Whales, with contributions from Aninimouse, Trevmillion, superninjakiwi,
WeendVidente, Drakasin and others (GPL-3.0, `LICENSE`; `data/motd.txt`
says GPLv3 too). It is Whales' unfinished ground-up rewrite of Cataclysm,
an early alpha, abandoned in 2016. From 0.2.0 (2026-11-13) this branch
continues it toward the finished game Whales planned: `VISION.md` sets out
that plan from Whales' notes and development blog, `ROADMAP.md` the
releases. The alpha as first packaged here is `v0+20140424-av.1`; our
continuation is versioned 0.2.0, 0.3.0 and on (our own game's scheme, since
the changes are now our own work on top of Whales'), 1.0.0 when the plan
is done.

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

## Ours

- **How to play it, at the top of README.md.** A short note on installing and
  starting it with av, or from a release archive; the original text follows
  unchanged.

## The continuation (0.2.0 on)

### 0.2.0: saving, loading and death

- **Saving and loading** (`savegame.cpp`, new): one save per world,
  `<save dir>/<world>/game.sav`, beside the world's map files: the turn,
  the map's position, the player (stats, skills, traits, needs, missions,
  status effects, chapters read, HP, everything worn and carried) and the
  monsters on the map, written to a temporary file and renamed. The world
  map and every loaded map sector are written with it. L on the first
  screen lists the worlds with a saved game; S saves; Q now asks "Save and
  quit?" (it asked "Commit suicide?" and ended the game); the game saves
  itself every two hours of game time.
- **Every game has a named world.** A new character without a chosen world
  got a nameless one whose map files went into the save folder's root; it
  now gets a random name (the world screen's own generator) and is kept.
  Starting a new character in a world that has a saved game asks first.
- **Death.** Nothing ever ended the game when the player's HP ran out: it
  went negative and play went on. A head or torso at 0 HP now ends it with
  a death screen (days survived, what did it), and deletes the save.
  `Player::last_damage` keeps the cause. `--test` adds a debug command,
  `k`, to test it.
- **Items** keep their contents (a loaded magazine, a bottle's water) when
  saved: `Item::save_data()` never wrote them. Loading never kept an item's
  on/off state (read, then dropped). Copying an item left its HP
  uninitialised, and assigning one did not copy HP or the on/off state.
- `DAYS(x)` in `time.h` was missing its closing bracket (unused until now).
- The start screen (`data/motd.txt`) says what this is and points bug
  reports to our repository; it gave Whales' email and IRC channel and
  called the game "not intended for general release".

### 0.3.0 and 0.4.0: washed ashore, and the way off the island

- **Sleep** (`$`, `survival.cpp`): there was no way to sleep and fatigue
  only grew. Sleep pays it back three times as fast as waking adds it and
  ends when rested (or after twelve hours).
- **Healing and starving**: one HP to each body part every two hours awake
  and every half hour asleep, none when starving or parched; past six days
  without food or two without water the torso takes damage until death.
- **Interruptions**: no activity was ever interrupted. Sleeping, reading
  and the rest now stop when a monster is in sight within 15 tiles (heard
  within 4 when asleep) or when you are hurt.
- **Help in the game** (`?`), and `data/help/interface.txt`, which the help
  menu listed as "Interface Tour" but never existed: the keys, staying
  alive, getting off the island. A first-day guide in the messages.
- **The way off the island** (`escape.cpp`): Whales' "bad win". The wreck
  (new terrain `wrecked boat`) lies on the shipwreck beach; examining it
  shows what it needs and does the work: hull, mast and sail, GPS unit,
  provisions, and then sailing, which ends the game with a victory screen.
  The boat's state is saved. New tools `radio` and `GPS unit` (new tool
  specials `radio` and `gps`); the radio is among the wreck's supplies,
  with a crowbar and batteries; GPS units, tarps and metal poles were
  added to item groups (tarps and poles were in none).
- The HUD clock is in 24-hour time: " PM" did not fit and was cut off.
- Fixes: `Entity::summons_used` and `parent_uid` were never set; a
  precedence slip in `Entity::remove_item()` compared a bool with the uid.

### 0.5.0: the Lab

- **World generation** places the Lab at the centre of the main island,
  its Substation beside it and the Power Plant in one corner
  (`Worldmap::place_lab()`, `lab.cpp`), with new world terrains `lab`,
  `lab substation` and `power plant` (added at the end of
  `world_terrain.dat` so older worlds keep their terrain ids) and their
  maps in `data/mapgen/lab.map`.
- **Machinery** (`lab.cpp`): new terrains `plant control panel`,
  `breaker panel`, `lab blast door` (opened by a signal no tool sends),
  `open blast door` and `lab core`, worked by examining them; a new item,
  `Lab passcard`, in the plant's office. The state is saved. The core
  ends the game with the good ending.
- The radio also picks up the Lab's recorded message.
- `--test` debug commands: `L` puts the machinery around the player with a
  wrench and a passcard, `k` kills the player.
