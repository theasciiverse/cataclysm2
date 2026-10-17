Cataclysm II
============

An unfinished alpha. Cataclysm II is Whales' ground-up rewrite of
Cataclysm, the roguelike Whales wrote from 2010 to 2013, worked on from
2013 to 2016 and then left: there is a world to explore, a character to make, items, monsters
and missions, but much is missing, and characters cannot be saved yet.
It is a separate project from Cataclysm: Dark Days Ahead, the community
fork of the first Cataclysm.

This build is the Asciiverse packaging of github.com/Whales/Cataclysm2,
from github.com/theasciiverse/cataclysm2; PATCHES.md there lists every
change. The game is under the GNU General Public License, version 3
(LICENSE).

Running it
----------

Start ./cataclysm2 in a terminal of at least 80x24. The game reads its
data from data/ and cuss/ beside the program. Worlds you make under
"W: Manage Worlds" are saved in $XDG_DATA_HOME/cataclysm2 (usually
~/.local/share/cataclysm2).

  cataclysm2 --help      the command-line options
  cataclysm2 --test      cheat commands (Z in the game)

H on the first screen opens the manual, with the keys; there is no help
key in the game itself. Q in the game ends it for good (characters are
not saved). README.upstream.md is the upstream readme.
