# Cataclysm II roadmap

Our continuation of Whales' Cataclysm II (see `VISION.md` for the plan
and where it comes from). Versions are 0.x.0 while unfinished, 1.0.0 when
everything below is in. Whales' alpha as packaged before is
`v0+20140424-av.1`; the continuation starts at 0.2.0.

## 0.2.0: a game you can leave and come back to

- [x] Save and load: one save per world, beside its map files
- [x] Every new game gets a named world of its own (saves need one)
- [x] S saves, Q saves and quits, autosave every two hours of game time
- [x] Death: a head or torso at 0 HP ends the game, with a death screen,
      and the save is deleted (it never could before: HP just went negative)
- [x] Items keep their contents, HP and on/off state when saved and copied
- [x] The start screen credits Whales and points bug reports here

## 0.3.0: washed ashore

Whales' early game (https://whalesdev.tumblr.com/post/74991567472).

- [x] The wreck: raw wood, a tool or two, a decent melee weapon, a radio
      (the crowbar and the radio joined Whales' planks, rope and food)
- [x] A help key in the game (`?`), a page of keys and goals, and a
      first-day guide in the messages
- [x] Sleep (`$`): there was no way to sleep; fatigue only grew
- [x] Natural healing, faster asleep; starvation and thirst that kill
- [x] Drink from fresh water and fill empty bottles there (examine it):
      there was no water but bottled water (0.6.0)
- [x] Long tasks (sleeping, reading, reloading) stop when danger comes near
      or you are hurt: nothing interrupted them
- [ ] More crashes and soft-locks found by playing, fixed (ongoing)

## 0.4.0: the way off the island

Whales' "bad win": a seaworthy boat and a working GPS.

- [x] The radio picks up the Saltcove coastal relay on the mainland, and
      says what the boat still needs
- [x] The wreck lies on the beach: examine it to patch the hull (planks,
      nails, a hammer), rig a mast and sail (tarps, rope, a metal pole),
      mount a GPS unit and stow food and drink
- [x] A GPS unit to find (camping and hiking shops, pawn shops, security
      gear), powered by batteries
- [x] Setting sail ends the game with a victory screen

## 0.5.0: the Lab

- [x] The Lab at the centre of the island (`L` on the island map), its
      Substation beside it (`s`), the Power Plant in one of the island's
      corners (`P`), with aliens in the Lab and zombies at the plant
- [x] Restart the plant (a wrench, two hours), close the Substation's
      breakers, show the passcard from the plant's office at the blast door
- [x] The Lab's core: shutting it down closes the seam the monsters come
      through, the good ending while no faction contests it
- [ ] The other corner Substations, the other three passcards (landing
      strip, silo, sea) and the Control Station, as in Whales' notes

## 0.6.0: people, first: the camps

- [x] Three survivor camps on the island (`C` on the island map), behind
      stockades with tents and a fire, their ground kept clear of monsters
- [x] Each camp's trader takes money, jewellery, medicine, batteries,
      bullets, liquor and computer parts on account, and sells food, water,
      medicine and the tools and materials both endings need
      (`data/trade.dat`)
- [x] Camp jobs: ask about work and a camp asks for something it needs;
      bring it to any camp for credit and some experience
- [x] Two raider camps (`R`): raiders and raider gunmen, hostile to
      everything, guarding what they have taken from others

## Toward 1.0.0: people

- [ ] Camp folk as people you can see and talk to, missions that send you
      somewhere
- [ ] The four factions (Cultists, Military, Naturalists, Scientists),
      their missions, rank, and their fight for the Lab
- [ ] Story mode and sandbox mode
- [ ] The rest of the Lab's machinery from Whales' notes: four corner
      substations, the other passcards, the Control Station
- [ ] Balance from people playing it: food clock, monster strength, prices
- [ ] Whales' unfinished item types: gun mods, bionics (`docs/todo.txt`)
- [x] Smaller saves: tile runs took a new world from 44 MB to 6 MB
