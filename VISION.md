# Cataclysm II: what Whales meant it to be, and how we finish it

Whales (the author of the first Cataclysm) started Cataclysm II in 2013 as a
ground-up rewrite with one big difference from Cataclysm 1: a plan for what
"finished" looks like. Whales left it in 2016 as an early alpha. This file sets
down that plan in our words, what exists, what is missing, and how this
branch finishes it. Sources: Whales' development blog
(https://whalesdev.tumblr.com/, 298 posts, read 2026-11-13 through
Tumblr's read API) and the notes in `docs/notes.txt` and `docs/todo.txt`.

## Whales' vision

From "Cataclysm II: An Introduction"
(https://whalesdev.tumblr.com/post/74182114978) and "Long-term plans for
Cataclysm 2" (https://whalesdev.tumblr.com/post/74991567472):

- **A winnable game.** Cataclysm 1 had no long-term goal, and that is
  where both Whales and the players ran out of direction. Cataclysm II has "a
  definitive winning ending", hard to reach, with a sandbox mode for people
  who only want to survive.
- **A finite world: an island.** You wake on a beach beside the wreck of
  your boat, having fled the mainland (`data/help/story.txt`). The island
  has biomes, towns joined by roads, z-levels you can see between.
- **Early game:** the wreck gives you some raw wood, a tool or two, a decent
  melee weapon and a radio. The map is unknown. Find water, food and shelter
  for the night. Beaches and fields are safe-ish, towns hold the goods and
  the zombies.
- **Pressure, not safety** (https://whalesdev.tumblr.com/post/91112281541):
  a faster food clock, scarce food in safe areas, an island that was looted
  when it was evacuated
  (https://whalesdev.tumblr.com/post/77820972042), no experience for
  killing (https://whalesdev.tumblr.com/post/91113197841), never total
  safety even in the wild.
- **Mid game:** four factions (Cultists, Military, Naturalists,
  Scientists; `docs/notes.txt`) and small camps of farmers and bandits.
  Trade, missions, rank, and enemies made by the company you keep.
- **Late and end game:** the factions fight for the Lab at the centre of
  the island. Unlocking it takes power (the Power Plant and the Lab's
  Substation), a passcard (four of them, hidden at set kinds of places) and
  the Control Station. Whoever holds the Lab starts a countdown to their
  faction's victory.
- **The "bad win"** (https://whalesdev.tumblr.com/post/86073588301):
  without joining any faction, build a boat that can survive the high seas
  and get a GPS network working again, and leave the island. "A win of last
  resort", but a win.

## What existed when work stopped (2016-07-01)

World map generation (island, biomes, towns, roads, map bonuses), the
data-driven engine (items, monsters, terrain, furniture, fields, mapgen in
`data/`), z-levels, field effects (fire, smoke), terrain with HP and
smashing, furniture you can drag, tools that send signals to the map (pry,
dig), firearms with a real accuracy model, hunger, thirst, fatigue, pain and
morale, status effects, skills, traits, professions, generated "personal
missions" (eat this, read that) for experience, and the shipwrecked start.

Not there: saving the player (the world's submaps were written to disk, but
"Load Character" was never written), any NPCs or factions, the Lab and its
power and passcards, the boat ending, crafting, and any way to win.

## How this branch finishes it

The finished game Whales described is years of work: four factions of
living NPCs with a strategy layer. We finish it in the order Whales planned
the work, from the beach outwards, so that every release is a whole
game you can play to an end:

1. **Saving and loading** first: a roguelike you cannot leave is not a game.
2. **The early game as Whales wrote it:** the wreck's supplies, the radio, the
   pressing clocks, and guidance for a new player.
3. **The way off the island:** Whales' "bad win" as the first ending. The radio
   gives it a thread to follow: a navigation beacon on the mainland can
   still be heard, and a working GPS unit can find it, so a seaworthy boat
   and a GPS unit get you home.
4. **The Lab:** power, passcards and the Control Station as world content;
   with nobody else contesting it yet, opening the Lab is the good ending.
5. **People:** camps to trade with, then the four factions, their missions
   and their claim on the Lab, as in the notes.

`ROADMAP.md` lists the releases. Choices made where Whales left no design
are recorded below as they are made.

## Choices made where Whales left no design

- **Where the save lives** (2026-11-13): one save per world, in the world's
  own folder beside its map files, `game.sav`. Death deletes it (a
  roguelike, as Whales' introduction says: "death is permanent"). Quitting
  saves.
