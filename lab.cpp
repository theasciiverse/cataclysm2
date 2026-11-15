// lab.cpp: the Lab at the centre of the island (Asciiverse, 2026).
//
// Whales' notes (docs/notes.txt, "The Island") make the Lab the goal of the
// whole game: it sits at the centre of the island, and opens only when it is
// powered (the Power Plant running and the Lab's Substation passing the
// power on) and a passcard is shown, or the Control Station opens it.  The
// factions fight over it; whoever holds it ends the game.  Until there are
// factions, the player alone can open it and shut down what drives the
// spawns, the Naturalists' victory in the notes ("the connection beyond has
// been severed").  Our simplifications are listed in VISION.md.

#include "game.h"
#include "worldmap.h"
#include "player.h"
#include "globals.h"
#include "window.h"
#include "stringfunc.h"
#include "monster_type.h"
#include "rng.h"
#include <sstream>

std::string wrap_tagged(std::string text, int width);  // escape.cpp
int text_menu(std::string text, std::vector<std::string> options);

static bool is_land(Worldmap* world, Point p)
{
  Worldmap_tile* tile = world->get_tile(p);
  return tile && tile->terrain && !tile->terrain->has_flag(WTF_WATER) &&
         tile->terrain->name != "beach_shipwreck";
}

static void add_spawn(Worldmap_tile* tile, std::string genus, int population)
{
  Monster_genus* gen = MONSTER_GENERA.lookup_name(genus);
  if (!gen || !tile) {
    return;
  }
  Monster_spawn spawn;
  spawn.genus = gen;
  spawn.population = population;
  tile->monsters.push_back(spawn);
}

void Worldmap::place_lab()
{
  if (islands.count(0) == 0 || islands[0].empty()) {
    return;
  }
  std::vector<Point>& land = islands[0];
// The Lab: the land tile nearest the island's centre.
  long sumx = 0, sumy = 0;
  for (int i = 0; i < land.size(); i++) {
    sumx += land[i].x;
    sumy += land[i].y;
  }
  Point centre(sumx / land.size(), sumy / land.size());
  Point lab(-1, -1);
  int best = 999999;
  for (int i = 0; i < land.size(); i++) {
    if (!is_land(this, land[i])) {
      continue;
    }
    int d = rl_dist(centre, land[i]);
    if (d < best) {
      best = d;
      lab = land[i];
    }
  }
  if (lab.x < 0) {
    return;
  }
  tiles[lab.x][lab.y].set_terrain("lab");
  tiles[lab.x][lab.y].monsters.clear();
  add_spawn(&(tiles[lab.x][lab.y]), "Chiratia", rng(6, 9));

// Its Substation, beside it.
  for (int n = 0; n < 8; n++) {
    int dx = (n % 3) - 1, dy = (n / 3) - 1;
    if (dx == 0 && dy == 0) {
      continue;
    }
    Point p(lab.x + dx * 2, lab.y + dy * 2);
    if (is_land(this, p)) {
      tiles[p.x][p.y].set_terrain("lab substation");
      break;
    }
  }

// The Power Plant, in one of the island's four corners.
  int sx = one_in(2) ? 1 : -1, sy = one_in(2) ? 1 : -1;
  Point plant(-1, -1);
  int far = -999999;
  for (int i = 0; i < land.size(); i++) {
    Point p = land[i];
    if (!is_land(this, p) || rl_dist(p, lab) < 10) {
      continue;
    }
    int score = sx * p.x + sy * p.y;
    if (score > far) {
      far = score;
      plant = p;
    }
  }
// Not right on the shore: step back toward the Lab a little if we can.
  for (int step = 0; plant.x >= 0 && step < 3; step++) {
    Point in(plant.x - sx, plant.y - sy);
    if (!is_land(this, in)) {
      break;
    }
    plant = in;
  }
  if (plant.x >= 0) {
    tiles[plant.x][plant.y].set_terrain("power plant");
    tiles[plant.x][plant.y].monsters.clear();
    add_spawn(&(tiles[plant.x][plant.y]), "zombies", rng(5, 8));
  }
}

/**** Working the Lab's machinery ****/

static int pending_corner = -1;   // which corner Substation is being worked

// Which corner Substation (0 to 2) the player stands in, or -1.
static int corner_here(Game* g)
{
  int wx = g->map->posx + g->player->pos.x / SUBMAP_SIZE;
  int wy = g->map->posy + g->player->pos.y / SUBMAP_SIZE;
  if (g->worldmap->get_name(wx, wy) != "substation") {
    return -1;
  }
  int n = 0;
  for (int x = 0; x < WORLDMAP_SIZE; x++) {
    for (int y = 0; y < WORLDMAP_SIZE; y++) {
      Worldmap_tile* t = g->worldmap->get_tile(x, y, false);
      if (t && t->terrain && t->terrain->name == "substation") {
        if (x == wx && y == wy) {
          return n < 3 ? n : 2;
        }
        n++;
      }
    }
  }
  return -1;
}

static const char* FACTION_NAMES[NUM_GROUPS] = {
  "the camps", "the raiders", "the Cultists", "the Military",
  "the Naturalists", "the Scientists"
};

static bool has_item_named(Player* pl, std::string name)
{
  name = no_caps(name);
  if (pl->weapon.is_real() && no_caps(pl->weapon.get_type()->name) == name) {
    return true;
  }
  for (int i = 0; i < pl->inventory.size(); i++) {
    if (pl->inventory[i].is_real() &&
        no_caps(pl->inventory[i].get_type()->name) == name) {
      return true;
    }
  }
  return false;
}

// Returns true if the terrain at pos is part of the Lab's machinery and was
// handled here.
bool Game::examine_lab_machinery(Tripoint pos)
{
  std::string ter = map->get_name(pos);
  if (ter == "plant control panel") {
    if (lab.plant_on && lab.holder >= 0 && lab.holder != member) {
      std::string q = std::string("The turbines hum.  ") +
                      FACTION_NAMES[lab.holder] + " hold the Lab on this \
power.  Shut the plant down and break their hold?";
      if (query_yn(q.c_str())) {
        add_msg("<c=ltgreen>You throw the turbines out of gear.  The lines \
to the Lab go dead, and with them %s's hold on it.<c=/>",
                FACTION_NAMES[lab.holder]);
        factions[lab.holder].progress = 60;
        hurt_standing(lab.holder, 30);
        lab.holder = -1;
        lab.plant_on = false;
        player->use_ap(100);
      }
    } else if (lab.plant_on) {
      popup("The turbines hum behind you.  The plant is running, and the \
lines north to the Lab carry power again.");
    } else if (!has_item_named(player, "wrench")) {
      popup(wrap_tagged("The control panel is dark, and the turbines are \
seized with rust and years.  With a <c=yellow>wrench<c=/> you could free them \
and bring the plant back to life.", 70).c_str());
    } else if (query_yn("Free the turbines and restart the plant?  (2 hours)")) {
      add_msg("You set to work on the turbines.");
      player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, HOURS(2) * 100, 10);
    }
    return true;
  }
  if (ter == "breaker panel" && corner_here(this) >= 0) {
    int c = corner_here(this);
    if (lab.corner_power & (1 << c)) {
      popup("The breakers are closed.  This corner of the island has power.");
    } else if (!lab.plant_on) {
      popup(wrap_tagged("The breakers are open, and nothing would come \
through them: the <c=yellow>power plant<c=/> isn't running.", 70).c_str());
    } else if (query_yn("Close the breakers and bring power to this corner \
of the island?")) {
      add_msg("You work the breakers closed one by one.");
      pending_corner = c;
      player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, MINUTES(10) * 100, 12);
    }
    return true;
  }
  if (ter == "lab remote switch") {
    if (lab.control_on) {
      popup("The switch is thrown.  On the panel, LAB ENTRANCE: RELEASED.");
    } else if (!lab.plant_on) {
      popup(wrap_tagged("A heavy switch under a glass cover: <c=yellow>LAB \
ENTRANCE, REMOTE RELEASE<c=/>.  The panel is dark.  It needs the <c=yellow>power \
plant<c=/> running.", 70).c_str());
    } else if (query_yn("Break the glass and throw the Lab's remote release?")) {
      add_msg("You throw the switch.  Somewhere to the south, bolts slide.");
      player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, MINUTES(2) * 100, 13);
    }
    return true;
  }
  if (ter == "breaker panel") {
    if (lab.substation_on) {
      popup("The breakers are closed.  Power runs on to the Lab.");
    } else if (!lab.plant_on) {
      popup(wrap_tagged("The breakers are open, but it would make no \
difference: no power reaches the substation.  Somewhere on the island, a \
<c=yellow>power plant<c=/> would have to run first.", 70).c_str());
    } else if (query_yn("Close the breakers and send power to the Lab?")) {
      add_msg("You work the breakers closed one by one.");
      player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, MINUTES(10) * 100, 11);
    }
    return true;
  }
  if (ter == "lab blast door") {
    if (!lab.plant_on || !lab.substation_on) {
      popup(wrap_tagged("A blast door a foot thick.  The card reader beside it \
is dead: no power reaches the Lab.  It needs the <c=yellow>power plant<c=/> \
running and the Lab's <c=yellow>substation<c=/> passing the power on.",
                        70).c_str());
    } else if (!lab.control_on) {
      popup(wrap_tagged("The card reader is lit, but a second panel beside \
it reads <c=ltred>REMOTE RELEASE NOT SET<c=/>.  The Lab's door answers to the \
<c=yellow>Control Station<c=/> as well as a card.", 70).c_str());
    } else if (!has_item_named(player, "Lab passcard")) {
      popup(wrap_tagged("The card reader beside the blast door glows red: \
<c=ltred>PASSCARD REQUIRED<c=/>.  Four were made; any one will do.",
                        70).c_str());
    } else {
      add_msg("You hold the passcard to the reader.  It chirps green.");
      map->apply_signal("unlock", pos, player);
      lab.open = true;
      player->use_ap(100);
    }
    return true;
  }
  if (ter == "lab core") {
    std::stringstream text;
    text << "<c=ltcyan>The Lab's core.<c=/>  Behind thick glass, a lattice " <<
            "of machinery holds something that is not quite there: a seam in " <<
            "the air that bends the light around it.  Every monster on the " <<
            "island came through seams like this one.  The console in front " <<
            "of you still answers.";
    popup(wrap_tagged(text.str(), 70).c_str());
    std::vector<std::string> opts;
    std::vector<int> acts;
    opts.push_back("Shut the core down and close the seam for good");
    acts.push_back(1);
    int rank_missions = (member >= 0) ? factions[member].missions : 0;
    if (!sandbox && member >= GROUP_CULTISTS && lab.holder < 0) {
      std::string o = std::string("Hand the Lab to ") + FACTION_NAMES[member];
      if (rank_missions < 4) {
        o += " (they don't trust you enough yet)";
      }
      opts.push_back(o);
      acts.push_back(2);
    }
    opts.push_back("Leave it be");
    acts.push_back(0);
    int pick = text_menu("The console waits.", opts);
    int act = (pick >= 0 && pick < acts.size()) ? acts[pick] : 0;
    if (act == 1) {
      lab_victory();
    } else if (act == 2) {
      if (rank_missions < 4) {
        popup(wrap_tagged(std::string("You radio ") + FACTION_NAMES[member] +
                          ".  \"You?  Hold it, and we'll talk when you've \
proven yourself.\"  (Do more of their errands.)", 66).c_str());
      } else {
        lab.holder = member;
        lab.countdown = time.get_turn() + DAYS(3);
        add_msg("<c=ltgreen>You radio %s, and their people pour in.  The \
Lab is theirs.  Hold out three days and the island is too.<c=/>",
                FACTION_NAMES[member]);
      }
    }
    return true;
  }
  return false;
}

void Game::finish_lab_work(int stage)
{
  if (stage == 10) {
    lab.plant_on = true;
    add_msg("<c=ltgreen>The turbines shudder, catch, and roar.  The power \
plant is running.<c=/>");
  } else if (stage == 12) {
    if (pending_corner >= 0) {
      lab.corner_power |= (1 << pending_corner);
    }
    pending_corner = -1;
    add_msg("<c=ltgreen>The last breaker closes.  Lights come on across \
this corner of the island.<c=/>");
  } else if (stage == 13) {
    lab.control_on = true;
    add_msg("<c=ltgreen>The panel reads LAB ENTRANCE: RELEASED.<c=/>");
  } else if (stage == 11) {
    lab.substation_on = true;
    add_msg("<c=ltgreen>The last breaker closes with a bang.  Power runs on \
to the Lab.<c=/>");
  }
}

void Game::lab_victory()
{
  int days = (time.get_turn() - Time(0, 0, 8, 1, SEASON_SPRING,
                                     STARTING_YEAR).get_turn()) / DAYS(1);
  std::string name = player->get_name();
  std::stringstream text;
  text << "<c=ltgreen>The seam closes.<c=/>\n\n" <<
    "The lattice spins down.  The bent light straightens, flickers and is " <<
    "gone, and for the first time since you woke on the beach the air does " <<
    "not hum.\n\n" <<
    "Nothing new will come through.  What is already on the island is " <<
    "still out there, but it is only what it is now, and fewer every " <<
    "year.  The island can be lived on.\n\n" <<
    name << " survived " << days_text(days) << " and won the island " <<
    "back.\n\n" <<
    "<c=dkgray>The factions will fight over the empty room for years.<c=/>";
  popup(wrap_tagged(text.str(), 66).c_str());
  delete_save();
  game_over = true;
  player->action_points = 0;
}
