Cataclysm II
============

Whales' sequel to Cataclysm (2013 to 2016), continued by the Asciiverse
from Whales' notes and development blog toward the finished game Whales
planned: you wake on the beach of an island beside the wreck of your boat,
and must survive it and find your way off. VISION.md and ROADMAP.md in the
repository say where it is going. It is a separate project from Cataclysm:
Dark Days Ahead, the community fork of the first Cataclysm.

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

H on the first screen opens the manual, with the keys. In the game, S
saves, Q saves and quits, and the game saves itself every two hours of
game time; L on the first screen carries on. Death is permanent: it ends
the game and deletes the save. README.upstream.md is the upstream readme.
