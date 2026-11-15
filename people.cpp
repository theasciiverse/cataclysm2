// people.cpp: people, the four factions and their errands (Asciiverse, 2026).
//
// Whales' notes (docs/notes.txt) put four factions on the island, the
// Cultists, the Military, the Naturalists and the Scientists, each wanting
// the Lab for its own ends, with "several smaller camps of survivors"
// beside them (https://whalesdev.tumblr.com/post/74991567472).  The engine
// had no people; here they are monsters of their own genera whose quarrels
// are decided by Monster::is_enemy() below, who talk when you walk into
// them, give errands that send you across the island, and race each other
// for the Lab.  Our simplifications are listed in VISION.md.

#include "game.h"
#include "worldmap.h"
#include "player.h"
#include "globals.h"
#include "window.h"
#include "stringfunc.h"
#include "monster_type.h"
#include "rng.h"
#include <sstream>
#include <cctype>

std::string wrap_tagged(std::string text, int width);           // escape.cpp
int text_menu(std::string text, std::vector<std::string> options);

/**** Who is who ****/

static const char* GROUP_GENUS[NUM_GROUPS] = {
  "survivors", "raiders", "cultists", "military", "naturalists", "scientists"
};

static const char* GROUP_NAME[NUM_GROUPS] = {
  "the camps", "the raiders", "the Cultists", "the Military",
  "the Naturalists", "the Scientists"
};

static const char* GROUP_BASE[NUM_GROUPS] = {
  "survivor camp", "raider camp", "cultist shrine", "military outpost",
  "naturalist commune", "science station"
};

static bool is_faction(int g)
{
  return g >= GROUP_CULTISTS && g <= GROUP_SCIENTISTS;
}

static int group_of(Entity* ent)
{
  if (!ent || !ent->is_monster()) {
    return GROUP_NONE;
  }
  Monster* mon = static_cast<Monster*>(ent);
  if (!mon->type || !mon->type->genus) {
    return GROUP_NONE;
  }
  std::string genus = mon->type->genus->name;
  for (int g = 0; g < NUM_GROUPS; g++) {
    if (genus == GROUP_GENUS[g]) {
      return g;
    }
  }
  return GROUP_NONE;
}

static bool is_alien(Entity* ent)
{
  if (!ent || !ent->is_monster()) {
    return false;
  }
  Monster* mon = static_cast<Monster*>(ent);
  return mon->type && mon->type->genus && mon->type->genus->name == "Chiratia";
}

// Feuds between groups, after the notes: the Cultists and the Military are
// at odds with everyone; the Naturalists and the Scientists with those two.
// The raiders fight all comers; the camps fight only the raiders.
static bool groups_feud(int a, int b)
{
  if (a == b) {
    return false;
  }
  if (a == GROUP_RAIDERS || b == GROUP_RAIDERS) {
    return true;
  }
  if (a == GROUP_SURVIVORS || b == GROUP_SURVIVORS) {
    return false;
  }
  if (a == GROUP_CULTISTS || b == GROUP_CULTISTS) {
    return true;
  }
  if (a == GROUP_MILITARY || b == GROUP_MILITARY) {
    return true;
  }
  return false;   // Naturalists and Scientists get along
}

bool Game::group_hostile_to_player(int g)
{
  if (g == GROUP_NONE) {
    return true;
  }
  if (g == GROUP_RAIDERS) {
    return true;
  }
  if (factions[g].standing <= -20) {
    return true;
  }
// Members of an enemy faction are killed on sight, except by the
// Naturalists, who welcome them (with suspicion) and by the camps.
  if (member != GROUP_NONE && member != g && is_faction(g) &&
      g != GROUP_NATURALISTS && groups_feud(member, g)) {
    return true;
  }
  return false;
}

bool Monster::is_enemy(Entity* ent)
{
  if (!ent || ent->uid == uid) {
    return false;
  }
  int me = group_of(this), them = group_of(ent);
  if (ent->is_you()) {
    if (me != GROUP_NONE) {
      return GAME.group_hostile_to_player(me);
    }
// The aliens spare the Cultists' own.
    if (is_alien(this) && GAME.member == GROUP_CULTISTS) {
      return false;
    }
    return true;
  }
  if (is_you()) {
    return true;
  }
  if (me != GROUP_NONE && them != GROUP_NONE) {
    return groups_feud(me, them);
  }
  if (me == GROUP_NONE && them == GROUP_NONE) {
    return Entity::is_enemy(ent);
  }
  if ((me == GROUP_CULTISTS && is_alien(ent)) ||
      (them == GROUP_CULTISTS && is_alien(this))) {
    return false;
  }
  return true;
}

void Game::hurt_standing(int g, int amount)
{
  if (g >= 0 && g < NUM_GROUPS) {
    factions[g].standing -= amount;
    if (member == g && factions[g].standing <= -20) {
      member = GROUP_NONE;
      add_msg("<c=ltred>%s cast you out.<c=/>", GROUP_NAME[g]);
    }
  }
}

/**** Names and directions ****/

static std::string person_name(Monster* mon)
{
  const char* first[16] = { "Ada", "Bram", "Cleo", "Dov", "Edda", "Finn",
    "Greta", "Hugo", "Iris", "Jonas", "Kerry", "Lars", "Mira", "Nils",
    "Orla", "Pavel" };
  const char* last[12] = { "Hale", "Moss", "Varga", "Lund", "Okafor",
    "Reyes", "Strand", "Quill", "Dahl", "Brandt", "Ferro", "Wilde" };
  int u = mon->uid < 0 ? -mon->uid : mon->uid;
  return std::string(first[(u * 7) % 16]) + " " + last[(u * 5 + 3) % 12];
}

static std::string direction_text(Point from, Point to)
{
  int dx = to.x - from.x, dy = to.y - from.y;
  int dist = rl_dist(from, to);
  if (dist == 0) {
    return "right here";
  }
  std::string ns = (dy * 2 < -dist) ? "north" : (dy * 2 > dist) ? "south" : "";
  std::string ew = (dx * 2 < -dist) ? "west" : (dx * 2 > dist) ? "east" : "";
  std::string dir = ns.empty() ? ew : (ew.empty() ? ns : ns + "-" + ew);
  std::stringstream ret;
  ret << "about " << dist << " squares " << dir << " on your island map";
  return ret.str();
}

static Point player_world(Game* g)
{
  return Point(g->map->posx + g->player->pos.x / SUBMAP_SIZE,
               g->map->posy + g->player->pos.y / SUBMAP_SIZE);
}

/**** Items carried ****/

static Item* carried(Player* pl, std::string name)
{
  name = no_caps(name);
  if (pl->weapon.is_real() && no_caps(pl->weapon.get_type()->name) == name) {
    return &(pl->weapon);
  }
  for (int i = 0; i < pl->inventory.size(); i++) {
    if (pl->inventory[i].is_real() &&
        no_caps(pl->inventory[i].get_type()->name) == name) {
      return &(pl->inventory[i]);
    }
  }
  return NULL;
}

static bool take_one(Player* pl, std::string name)
{
  name = no_caps(name);
  if (pl->weapon.is_real() && no_caps(pl->weapon.get_type()->name) == name) {
    pl->weapon = Item();
    return true;
  }
  for (int i = 0; i < pl->inventory.size(); i++) {
    Item& it = pl->inventory[i];
    if (it.is_real() && no_caps(it.get_type()->name) == name) {
      if (it.count > 1) {
        it.count--;
      } else {
        pl->inventory.erase(pl->inventory.begin() + i);
      }
      return true;
    }
  }
  return false;
}

/**** Finding places on the island ****/

static std::vector<Point> tiles_named(Worldmap* world, std::string name)
{
  std::vector<Point> ret;
  for (int x = 0; x < WORLDMAP_SIZE; x++) {
    for (int y = 0; y < WORLDMAP_SIZE; y++) {
      Worldmap_tile* t = world->get_tile(x, y, false);
      if (t && t->terrain && t->terrain->name == name) {
        ret.push_back(Point(x, y));
      }
    }
  }
  return ret;
}

static bool is_building(Worldmap_tile* t)
{
  if (!t || !t->terrain) {
    return false;
  }
  std::string n = t->terrain->name;
  const char* kinds[] = { "house", "store", "bank", "bar", "library",
    "museum", "gas station", "church", "school", "pharmacy", "hospital",
    "apartment", "office", "restaurant", "police", NULL };
  for (int i = 0; kinds[i]; i++) {
    if (n.find(kinds[i]) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static Point nearest_named(Worldmap* world, Point from, std::string name)
{
  std::vector<Point> all = tiles_named(world, name);
  Point best(-1, -1);
  int bestd = 999999;
  for (int i = 0; i < all.size(); i++) {
    int d = rl_dist(from, all[i]);
    if (d < bestd) {
      bestd = d;
      best = all[i];
    }
  }
  return best;
}

/**** World generation: bases, corner Substations, bunker, wreck, station ****/

static bool far_from(Point p, std::vector<Point>& others, int dist)
{
  for (int i = 0; i < others.size(); i++) {
    if (rl_dist(p, others[i]) < dist) {
      return false;
    }
  }
  return true;
}

static void add_spawn_to(Worldmap_tile* tile, std::string genus, int pop)
{
  Monster_genus* gen = MONSTER_GENERA.lookup_name(genus);
  if (!gen || !tile) {
    return;
  }
  Monster_spawn spawn;
  spawn.genus = gen;
  spawn.population = pop;
  tile->monsters.push_back(spawn);
}

void Worldmap::place_factions()
{
  if (islands.count(0) == 0 || islands[0].empty()) {
    return;
  }
  std::vector<Point>& land = islands[0];
  std::vector<Point> lab = tiles_named(this, "lab");
  std::vector<Point> plant = tiles_named(this, "power plant");
  if (lab.empty()) {
    return;
  }
  Point L = lab[0];
  std::vector<Point> taken = lab;
  for (int i = 0; i < plant.size(); i++) {
    taken.push_back(plant[i]);
  }
  std::vector<Point> camps = tiles_named(this, "survivor camp");
  std::vector<Point> raiders = tiles_named(this, "raider camp");
  taken.insert(taken.end(), camps.begin(), camps.end());
  taken.insert(taken.end(), raiders.begin(), raiders.end());

  std::vector<Point> open;
  for (int i = 0; i < land.size(); i++) {
    Worldmap_tile* t = get_tile(land[i]);
    if (t && t->terrain && (t->terrain->name == "field" ||
                            t->terrain->name == "forest")) {
      open.push_back(land[i]);
    }
  }
  if (open.empty()) {
    return;
  }

// The four bases, well apart from each other and from the Lab.
  const char* bases[4] = { "cultist shrine", "military outpost",
                           "naturalist commune", "science station" };
  const char* genera[4] = { "cultists", "military", "naturalists",
                            "scientists" };
  for (int b = 0; b < 4; b++) {
    for (int tries = 0; tries < 800; tries++) {
      Point p = open[rng(0, open.size() - 1)];
      if (rl_dist(p, L) < 8 || !far_from(p, taken, 10)) {
        continue;
      }
      tiles[p.x][p.y].set_terrain(bases[b]);
      tiles[p.x][p.y].monsters.clear();
      add_spawn_to(&(tiles[p.x][p.y]), genera[b], rng(5, 7));
      taken.push_back(p);
      break;
    }
  }

// The corner Substations: the three corners the Power Plant didn't take.
  if (!plant.empty()) {
    int psx = plant[0].x >= L.x ? 1 : -1, psy = plant[0].y >= L.y ? 1 : -1;
    for (int c = 0; c < 4; c++) {
      int sx = (c & 1) ? 1 : -1, sy = (c & 2) ? 1 : -1;
      if (sx == psx && sy == psy) {
        continue;
      }
      Point best(-1, -1);
      int far = -999999;
      for (int i = 0; i < land.size(); i++) {
        Point p = land[i];
        Worldmap_tile* t = get_tile(p);
        if (!t || !t->terrain || t->terrain->has_flag(WTF_WATER) ||
            t->terrain->name.find("beach") != std::string::npos ||
            !far_from(p, taken, 4)) {
          continue;
        }
        int score = sx * (p.x - L.x) + sy * (p.y - L.y);
        if (score > far) {
          far = score;
          best = p;
        }
      }
      if (best.x >= 0) {
        tiles[best.x][best.y].set_terrain("substation");
        tiles[best.x][best.y].monsters.clear();
        add_spawn_to(&(tiles[best.x][best.y]), "zombies", rng(2, 3));
        taken.push_back(best);
      }
    }
  }

// The bunker (Passcard A), out in the wild.
  for (int tries = 0; tries < 800; tries++) {
    Point p = open[rng(0, open.size() - 1)];
    if (rl_dist(p, L) < 10 || !far_from(p, taken, 6)) {
      continue;
    }
    tiles[p.x][p.y].set_terrain("bunker");
    tiles[p.x][p.y].monsters.clear();
    add_spawn_to(&(tiles[p.x][p.y]), "zombies", rng(3, 5));
    taken.push_back(p);
    break;
  }

// The wreck on a far shore (Passcard C, at sea).
  std::vector<Point> beaches = tiles_named(this, "beach");
  std::vector<Point> more = tiles_named(this, "salt marsh");
  beaches.insert(beaches.end(), more.begin(), more.end());
  more = tiles_named(this, "mangrove");
  beaches.insert(beaches.end(), more.begin(), more.end());
  for (int tries = 0; tries < 1500 && !beaches.empty(); tries++) {
    Point p = beaches[rng(0, beaches.size() - 1)];
    int near = tries < 800 ? 10 : 4, apart = tries < 800 ? 6 : 2;
    if (rl_dist(p, L) < near || !far_from(p, taken, apart)) {
      continue;
    }
    tiles[p.x][p.y].set_terrain("derelict");
    tiles[p.x][p.y].monsters.clear();
    add_spawn_to(&(tiles[p.x][p.y]), "crabs", rng(2, 3));
    taken.push_back(p);
    break;
  }

// The Control Station, a short walk from the Lab.
  for (int tries = 0; tries < 800; tries++) {
    Point p = land[rng(0, land.size() - 1)];
    Worldmap_tile* t = get_tile(p);
    int d = rl_dist(p, L);
    if (d < 4 || d > 9 || !t || !t->terrain ||
        t->terrain->has_flag(WTF_WATER) ||
        t->terrain->name.find("beach") != std::string::npos ||
        !far_from(p, taken, 3)) {
      continue;
    }
    tiles[p.x][p.y].set_terrain("control station");
    tiles[p.x][p.y].monsters.clear();
    add_spawn_to(&(tiles[p.x][p.y]), "Chiratia", rng(2, 3));
    taken.push_back(p);
    break;
  }
}

/**** A new game: hidden passcards ****/

// Passcard B lies in the largest town, Passcard D in a house anywhere
// (Whales' notes; A is in the bunker and C in the wreck, by their maps).
static void plant_passcards(Game* g)
{
  g->planted.clear();
  Worldmap* world = g->worldmap;
  std::vector<Point> buildings;
  for (int x = 0; x < WORLDMAP_SIZE; x++) {
    for (int y = 0; y < WORLDMAP_SIZE; y++) {
      if (is_building(world->get_tile(x, y, false))) {
        buildings.push_back(Point(x, y));
      }
    }
  }
  if (buildings.empty()) {
    return;
  }
  int best = -1, bestn = -1;
  for (int i = 0; i < buildings.size(); i++) {
    int n = 0;
    for (int j = 0; j < buildings.size(); j++) {
      if (rl_dist(buildings[i], buildings[j]) <= 3) {
        n++;
      }
    }
    if (n > bestn) {
      bestn = n;
      best = i;
    }
  }
  Planted_item b;
  b.world = buildings[best];
  b.item = "Lab passcard";
  b.placed = false;
  g->planted.push_back(b);
  Planted_item d;
  d.world = buildings[rng(0, buildings.size() - 1)];
  d.item = "Lab passcard";
  d.placed = false;
  g->planted.push_back(d);
}

void people_new_game(Game* g)   // game.cpp, setup_new_game()
{
  for (int i = 0; i < NUM_GROUPS; i++) {
    g->factions[i] = Faction_state();
  }
  g->member = GROUP_NONE;
  g->job = Job();
  plant_passcards(g);
}

/**** Errands ****/

static const char* place_label(std::string ter)
{
  if (ter == "raider camp") return "the raider camp";
  if (ter == "bunker") return "the old bunker";
  if (ter == "derelict") return "the wreck on the far shore";
  if (ter == "control station") return "the Control Station";
  if (ter == "substation") return "the substation";
  if (ter == "lab") return "the Lab";
  if (ter == "power plant") return "the Power Plant";
  if (ter == "cultist shrine") return "the Cultists' shrine";
  if (ter == "military outpost") return "the Military outpost";
  if (ter == "naturalist commune") return "the Naturalists' commune";
  if (ter == "science station") return "the Scientists' station";
  if (ter == "survivor camp") return "the farming camp";
  return "that place";
}

static int genus_population(Worldmap_tile* t, std::string genus)
{
  int n = 0;
  for (int i = 0; t && i < t->monsters.size(); i++) {
    if (t->monsters[i].genus && t->monsters[i].genus->name == genus) {
      n += t->monsters[i].population;
    }
  }
  return n;
}

static bool make_job(Game* g, int giver)
{
  Job job;
  job.giver = giver;
  Point here = player_world(g);
  std::vector<int> kinds;
  kinds.push_back(JOB_SCOUT);
  kinds.push_back(JOB_FETCH);
  kinds.push_back(JOB_DELIVER);
  if (!tiles_named(g->worldmap, "raider camp").empty()) {
    kinds.push_back(JOB_CLEAR);
  }
  if ((giver == GROUP_MILITARY || giver == GROUP_SCIENTISTS) &&
      g->lab.corner_power != 7 && !tiles_named(g->worldmap, "substation").empty()) {
    kinds.push_back(JOB_POWER);
    kinds.push_back(JOB_POWER);
  }
  job.kind = kinds[rng(0, kinds.size() - 1)];
  job.reward = rng(20, 40);

  if (job.kind == JOB_SCOUT) {
    const char* sites[] = { "control station", "bunker", "derelict",
      "substation", "raider camp", "power plant", "cultist shrine",
      "military outpost", "naturalist commune", "science station", NULL };
    std::vector<Point> options;
    std::vector<std::string> names;
    for (int i = 0; sites[i]; i++) {
      if (std::string(sites[i]) == GROUP_BASE[giver]) {
        continue;
      }
      std::vector<Point> at = tiles_named(g->worldmap, sites[i]);
      for (int k = 0; k < at.size(); k++) {
        options.push_back(at[k]);
        names.push_back(sites[i]);
      }
    }
    if (options.empty()) {
      return false;
    }
    int pick = rng(0, options.size() - 1);
    job.target = options[pick];
    job.place = place_label(names[pick]);
  } else if (job.kind == JOB_CLEAR) {
    std::vector<Point> at = tiles_named(g->worldmap, "raider camp");
    std::vector<Point> live;
    for (int i = 0; i < at.size(); i++) {
      if (genus_population(g->worldmap->get_tile(at[i]), "raiders") > 0) {
        live.push_back(at[i]);
      }
    }
    if (live.empty()) {
      return false;
    }
    job.target = live[rng(0, live.size() - 1)];
    job.place = "the raider camp";
    job.reward += 20;
  } else if (job.kind == JOB_FETCH) {
    std::vector<Point> spots;
    for (int x = 0; x < WORLDMAP_SIZE; x++) {
      for (int y = 0; y < WORLDMAP_SIZE; y++) {
        Point p(x, y);
        int d = rl_dist(here, p);
        if (d >= 6 && d <= 30 && is_building(g->worldmap->get_tile(x, y, false))) {
          spots.push_back(p);
        }
      }
    }
    if (spots.empty()) {
      return false;
    }
    job.target = spots[rng(0, spots.size() - 1)];
    job.place = "a building in a town";
    Planted_item case_;
    case_.world = job.target;
    case_.item = "supply case";
    case_.placed = false;
    g->planted.push_back(case_);
  } else if (job.kind == JOB_DELIVER) {
    std::vector<int> to;
    for (int t = 0; t < NUM_GROUPS; t++) {
      if (t != giver && t != GROUP_RAIDERS && !groups_feud(giver, t) &&
          !tiles_named(g->worldmap, GROUP_BASE[t]).empty()) {
        to.push_back(t);
      }
    }
    if (to.empty()) {
      return false;
    }
    job.to = to[rng(0, to.size() - 1)];
    job.target = nearest_named(g->worldmap, here, GROUP_BASE[job.to]);
    job.place = place_label(GROUP_BASE[job.to]);
    Item letter(ITEM_TYPES.lookup_name("sealed letter"));
    if (!g->player->add_item(letter)) {
      g->map->add_item(letter, g->player->pos);
    }
  } else if (job.kind == JOB_POWER) {
    std::vector<Point> subs = tiles_named(g->worldmap, "substation");
    std::vector<int> off;
    for (int i = 0; i < subs.size() && i < 3; i++) {
      if (!(g->lab.corner_power & (1 << i))) {
        off.push_back(i);
      }
    }
    if (off.empty()) {
      return false;
    }
    job.bit = off[rng(0, off.size() - 1)];
    job.target = subs[job.bit];
    job.place = "the substation";
    job.reward += 15;
  }
  g->job = job;
  return true;
}

std::string Game::job_text()
{
  if (!job.active()) {
    return "";
  }
  std::stringstream t;
  Point here = player_world(this);
  std::string where = direction_text(here, job.target);
  if (job.done && job.kind != JOB_DELIVER) {
    t << "Done: report back to " << GROUP_NAME[job.giver] << ".";
    return t.str();
  }
  switch (job.kind) {
    case JOB_SCOUT:
      t << "Look over " << job.place << ", " << where << ".";
      break;
    case JOB_CLEAR:
      t << "Clear the raiders out of " << job.place << ", " << where << ".";
      break;
    case JOB_FETCH:
      t << "Fetch the supply case left in " << job.place << ", " << where <<
           ", and bring it to " << GROUP_NAME[job.giver] << ".";
      break;
    case JOB_DELIVER:
      t << "Carry the sealed letter to " << GROUP_NAME[job.to] << " at " <<
           job.place << ", " << where << ".";
      break;
    case JOB_POWER:
      t << "Close the breakers at " << job.place << ", " << where <<
           " (the Power Plant has to be running).";
      break;
  }
  return t.str();
}

static std::string errand_offer(int giver, int kind)
{
  switch (giver) {
    case GROUP_CULTISTS:
      return "\"The Coming needs eyes and hands.  Do this, and the garden will \
know your name.\"";
    case GROUP_MILITARY:
      return kind == JOB_POWER ?
        "\"Power is control.  Get that substation online, soldier.\"" :
        "\"Orders.  Carry them out and report back.\"";
    case GROUP_NATURALISTS:
      return "\"We vote on most things, but this one's easy: somebody has to \
go, and you're here.\"";
    case GROUP_SCIENTISTS:
      return kind == JOB_POWER ?
        "\"Our instruments need stable power.  The substation, please.\"" :
        "\"Data first.  We'll make it worth your while.\"";
    default:
      return "\"There's a thing that needs doing, if you're willing.\"";
  }
}

static void reward_job(Game* g)
{
  int giver = g->job.giver;
  g->trade_credit += g->job.reward;
  g->factions[giver].standing += 10;
  g->factions[giver].missions++;
  g->player->gain_experience(40);
  if (is_faction(giver) && g->member == giver && !g->sandbox) {
    g->factions[giver].progress += 5;
  }
  g->add_msg("<c=ltgreen>%s pay you %d on account.<c=/>", GROUP_NAME[giver],
             g->job.reward);
  g->job = Job();
}

/**** Talking ****/

static int rank_of(Game* g, int f)
{
  int m = g->factions[f].missions;
  return m >= 7 ? 3 : m >= 4 ? 2 : m >= 2 ? 1 : 0;
}

static std::string rank_name(int f, int rank)
{
  const char* names[4][4] = {
    { "seeker", "acolyte", "adept", "hierophant's hand" },
    { "recruit", "private", "sergeant", "lieutenant" },
    { "newcomer", "neighbour", "voice", "trusted voice" },
    { "assistant", "fellow", "senior fellow", "associate director" }
  };
  return names[f - GROUP_CULTISTS][rank];
}

static std::string rumour(Game* g)
{
  Point here = player_world(g);
  std::vector<std::string> lines;
  Point p;
  p = nearest_named(g->worldmap, here, "bunker");
  if (p.x >= 0) {
    lines.push_back("\"There's an old bunker " + direction_text(here, p) +
                    ".  They say the Lab people kept keys there.\"");
  }
  p = nearest_named(g->worldmap, here, "derelict");
  if (p.x >= 0) {
    lines.push_back("\"A ship ran aground on the far shore, " +
                    direction_text(here, p) + ".  Lab people were aboard, \
running for it.\"");
  }
  p = nearest_named(g->worldmap, here, "control station");
  if (p.x >= 0) {
    lines.push_back("\"The Lab's door has a remote switch in a little \
station " + direction_text(here, p) + ".  The aliens nest there now.\"");
  }
  p = nearest_named(g->worldmap, here, "power plant");
  if (p.x >= 0) {
    lines.push_back("\"Nothing in the Lab works without the Power Plant, " +
                    direction_text(here, p) + ".  You'd need a wrench.\"");
  }
  for (int i = 0; i < g->planted.size(); i++) {
    if (g->planted[i].item == "Lab passcard" && !g->planted[i].placed) {
      lines.push_back("\"Lab staff fled into town when it went bad.  One of \
them is still in a building " + direction_text(here, g->planted[i].world) +
                      ", I'd bet, card and all.\"");
    }
  }
  for (int f = GROUP_CULTISTS; f <= GROUP_SCIENTISTS; f++) {
    p = nearest_named(g->worldmap, here, GROUP_BASE[f]);
    if (p.x >= 0) {
      lines.push_back(std::string("\"") + GROUP_NAME[f] + " hold " +
                      place_label(GROUP_BASE[f]) + ", " +
                      direction_text(here, p) + ".\"");
    }
  }
  lines.push_back("\"Four passcards were made for the Lab's door.  Any one \
will do, if it's powered and the Control Station's switch is thrown.\"");
  return lines[rng(0, lines.size() - 1)];
}

static std::string lab_race(Game* g)
{
  std::stringstream t;
  if (g->lab.holder >= 0) {
    t << GROUP_NAME[g->lab.holder] << " hold the Lab.  " <<
         days_text((g->lab.countdown - g->time.get_turn()) / DAYS(1) + 1) <<
         " until it's theirs for good, they say.";
    return t.str();
  }
  int best = GROUP_CULTISTS;
  for (int f = GROUP_CULTISTS; f <= GROUP_SCIENTISTS; f++) {
    if (g->factions[f].progress > g->factions[best].progress) {
      best = f;
    }
  }
  int p = g->factions[best].progress;
  t << "Nobody holds the Lab yet.  " << GROUP_NAME[best] << " are " <<
       (p < 30 ? "a long way from it" : p < 60 ? "working toward it" :
        p < 85 ? "getting close" : "nearly there") << ".";
  return t.str();
}

static std::string faction_offer(int f)
{
  switch (f) {
    case GROUP_CULTISTS: return "\"Join the garden.  The aliens will know you \
as one of ours and let you be.  Our counter keeps what the faithful need.\"";
    case GROUP_MILITARY: return "\"Serve, and you'll be armed: our counter \
has guns and ammunition for our own.\"";
    case GROUP_NATURALISTS: return "\"Anyone can join.  We share: food, \
water, rope and tarps at our store, for very little.\"";
    case GROUP_SCIENTISTS: return "\"We're careful whom we trust.  Members \
get our equipment: batteries, tools, a GPS unit if you need one.\"";
  }
  return "";
}

void Game::talk_to(Monster* mon)
{
  int g = group_of(mon);
  if (g == GROUP_NONE || group_hostile_to_player(g)) {
    player->attack(mon);
    return;
  }
  std::string name = person_name(mon);
  while (true) {
    std::stringstream text;
    std::string kind = mon->get_name();
    bool vowel = !kind.empty() && std::string("aeiouAEIOU").find(kind[0]) !=
                 std::string::npos;
    text << "<c=white>" << name << "<c=/>, " << (vowel ? "an " : "a ") <<
            kind << " of " << GROUP_NAME[g] << "." << std::endl << std::endl;
    std::vector<std::string> options;
    std::vector<int> acts;   // 0 leave 1 work 2 report 3 news 4 join 5 lab 6 attack 7 deliver
    if (is_faction(g)) {
      if (member == g) {
        std::string r = rank_name(g, rank_of(this, g));
        r[0] = toupper(r[0]);
        text << "\"" << r << ", you're back.\"";
      } else if (member != GROUP_NONE) {
        text << "\"You run with " << GROUP_NAME[member] << ".  Keep your hands " <<
                "where I can see them.\"";
      } else {
        text << "\"You're not one of ours.  Not yet, anyway.\"";
      }
    } else {
      text << "\"Stranger.  You look like you've been sleeping rough.\"";
    }
    if (job.active()) {
      text << std::endl << std::endl << "<c=ltgray>Your errand: " << job_text() <<
              "<c=/>";
    }
    if (job.active() && job.kind == JOB_DELIVER && job.to == g &&
        carried(player, "sealed letter")) {
      options.push_back("Hand over the sealed letter");
      acts.push_back(7);
    }
    if (job.active() && job.giver == g &&
        (job.done || (job.kind == JOB_FETCH && carried(player, "supply case")))) {
      options.push_back("Report: the errand is done");
      acts.push_back(2);
    }
    if (!job.active() && (g == GROUP_SURVIVORS || member == g ||
                          (is_faction(g) && member == GROUP_NONE))) {
      options.push_back("Ask about work");
      acts.push_back(1);
    }
    if (is_faction(g) && member == GROUP_NONE) {
      options.push_back("Ask about joining");
      acts.push_back(4);
    }
    options.push_back("Ask what they know");
    acts.push_back(3);
    if (is_faction(g) || g == GROUP_SURVIVORS) {
      options.push_back("Ask about the Lab");
      acts.push_back(5);
    }
    options.push_back("Attack them");
    acts.push_back(6);
    options.push_back("Leave");
    acts.push_back(0);

    int pick = text_menu(text.str(), options);
    int act = (pick >= 0 && pick < acts.size()) ? acts[pick] : 0;
    if (act == 0) {
      return;
    }
    if (act == 1) {
      if (is_faction(g) && member != g && factions[g].standing < 0) {
        popup("\"Not for you.  Not after what you did.\"");
      } else if (make_job(this, g)) {
        std::string s = errand_offer(g, job.kind) + "\n\n" + job_text();
        popup(wrap_tagged(s, 66).c_str());
        if (job.kind == JOB_DELIVER) {
          add_msg("You take the sealed letter.");
        }
      } else {
        popup("\"Nothing right now.  Come back later.\"");
      }
    } else if (act == 2) {
      if (job.kind == JOB_FETCH && !take_one(player, "supply case")) {
        popup("\"Where's the case?\"");
        continue;
      }
      reward_job(this);
    } else if (act == 7) {
      take_one(player, "sealed letter");
      factions[g].standing += 5;
      job.done = true;
      if (job.giver == g) {
        reward_job(this);
      } else {
        popup(wrap_tagged("They break the seal and read.  \"Tell them we've \
got it.\"  The errand is done: report back to " +
                          std::string(GROUP_NAME[job.giver]) + ".", 66).c_str());
      }
    } else if (act == 3) {
      popup(wrap_tagged(rumour(this), 66).c_str());
    } else if (act == 5) {
      popup(wrap_tagged("\"" + lab_race(this) + "\"", 66).c_str());
    } else if (act == 4) {
      std::string offer = faction_offer(g);
      bool ok = true;
      std::string refusal;
      if (g == GROUP_CULTISTS && factions[g].standing < 20) {
        ok = false;
        refusal = "\"Prove yourself to the garden first.  Do work for us.\"";
      } else if (g == GROUP_SCIENTISTS && factions[g].standing < 10) {
        ok = false;
        refusal = "\"We'd need to know you better.  Help us with something \
first.\"";
      }
      if (!ok) {
        popup(wrap_tagged(offer + "\n\n" + refusal, 66).c_str());
      } else if (popup(wrap_tagged(offer, 66).c_str()), query_yn("Join %s?", GROUP_NAME[g])) {
        member = g;
        factions[g].standing += 5;
        std::string welcome = g == GROUP_MILITARY ?
          "They search you, ask a hundred questions, and hand you a jacket \
with a stripe on it.  \"Welcome to the Military.\"" :
          g == GROUP_CULTISTS ? "They paint the spiral on your forehead with \
something that smells of mushrooms.  \"The garden knows you now.\"" :
          g == GROUP_NATURALISTS ? "There's a vote.  It's close, but you're \
in.  Someone hands you a bowl of beans." :
          "They issue you a badge and a password you are not to write down.";
        popup(wrap_tagged(welcome, 66).c_str());
        for (int f = GROUP_CULTISTS; f <= GROUP_SCIENTISTS; f++) {
          if (f != g && groups_feud(f, g)) {
            factions[f].standing -= 10;
          }
        }
      }
    } else if (act == 6) {
      if (query_yn("Really attack %s?", name.c_str())) {
        hurt_standing(g, 40);
        player->attack(mon);
        return;
      }
    }
  }
}

/**** Every turn ****/

void Game::people_turn()
{
  int turn = time.get_turn();
  if (turn % 10 != 0) {
    return;
  }
  Point here = player_world(this);

// Hidden things appear once their square is on the map.
  for (int i = 0; i < planted.size(); i++) {
    Planted_item& pl = planted[i];
    if (pl.placed) {
      continue;
    }
    int sx = pl.world.x - map->posx, sy = pl.world.y - map->posy;
    if (sx < 0 || sy < 0 || sx >= MAP_SIZE || sy >= MAP_SIZE) {
      continue;
    }
    std::vector<Tripoint> spots;
    for (int x = sx * SUBMAP_SIZE; x < (sx + 1) * SUBMAP_SIZE; x++) {
      for (int y = sy * SUBMAP_SIZE; y < (sy + 1) * SUBMAP_SIZE; y++) {
        if (map->move_cost(x, y, 0) > 0) {
          std::string ter = map->get_name(x, y, 0);
          bool inside = ter.find("floor") != std::string::npos;
          spots.push_back(Tripoint(x, y, 0));
          if (inside) {     // weight floors: things are kept indoors
            spots.push_back(Tripoint(x, y, 0));
            spots.push_back(Tripoint(x, y, 0));
          }
        }
      }
    }
    if (spots.empty()) {
      continue;
    }
    Item_type* type = ITEM_TYPES.lookup_name(pl.item);
    if (type) {
      map->add_item(Item(type), spots[rng(0, spots.size() - 1)]);
    }
    pl.placed = true;
  }

// Errands.
  if (job.active() && !job.done) {
    bool finished = false;
    if (job.kind == JOB_SCOUT && here == job.target) {
      finished = true;
      add_msg("<c=ltgreen>You have had a good look at %s.<c=/>",
              job.place.c_str());
    } else if (job.kind == JOB_POWER && (lab.corner_power & (1 << job.bit))) {
      finished = true;
      add_msg("<c=ltgreen>The substation hums with power.<c=/>");
    } else if (job.kind == JOB_FETCH && carried(player, "supply case")) {
      finished = true;
      add_msg("<c=ltgreen>You have the supply case.<c=/>");
    } else if (job.kind == JOB_CLEAR && rl_dist(here, job.target) <= 1) {
      Worldmap_tile* t = worldmap->get_tile(job.target);
      bool alive = genus_population(t, "raiders") > 0;
      for (std::list<Entity*>::iterator it = entities.instances.begin();
           !alive && it != entities.instances.end(); it++) {
        Entity* ent = *it;
        if (!ent->dead && group_of(ent) == GROUP_RAIDERS) {
          Point w(map->posx + ent->pos.x / SUBMAP_SIZE,
                  map->posy + ent->pos.y / SUBMAP_SIZE);
          if (w == job.target) {
            alive = true;
          }
        }
      }
      if (!alive) {
        finished = true;
        add_msg("<c=ltgreen>The raider camp is quiet.  It's done.<c=/>");
      }
    }
    if (finished) {
      job.done = true;
      add_msg("Report back to %s.", GROUP_NAME[job.giver]);
    }
  }

// The world map marks where the errand leads.
  worldmap->marks.clear();
  if (job.active() && !job.done) {
    worldmap->marks.push_back(job.target);
  }

  if (sandbox || !factions_on) {
    return;
  }

// The factions' race for the Lab, once a day at dawn.
  if (turn % DAYS(1) == HOURS(6) % DAYS(1)) {
    faction_dawn();
  }
  if (lab.holder >= 0 && turn >= lab.countdown) {
    if (lab.holder == member) {
      faction_victory(lab.holder);
    } else {
      lab_lost(lab.holder);
    }
  }
}

void Game::faction_dawn()
{
  int turn = time.get_turn();
  if (lab.holder < 0) {
    for (int f = GROUP_CULTISTS; f <= GROUP_SCIENTISTS; f++) {
      factions[f].progress += rng(1, 3);
    }
    for (int f = GROUP_CULTISTS; f <= GROUP_SCIENTISTS; f++) {
      if (factions[f].progress >= 100) {
        lab.holder = f;
        lab.countdown = turn + DAYS(member == f ? 3 : 7);
        lab.plant_on = true;        // they got the power running to take it
        lab.substation_on = true;
        if (member == f) {
          add_msg("<c=ltgreen>The radio crackles: %s have taken the Lab!  \
Hold out three days and the island is ours.<c=/>", GROUP_NAME[f]);
        } else {
          add_msg("<c=ltred>The radio crackles: %s have taken the Lab.  In \
seven days it will be theirs for good, unless someone cuts its power.<c=/>",
                  GROUP_NAME[f]);
        }
        break;
      }
    }
  }
}

void Game::faction_news(std::stringstream& text)
{
  if (sandbox || !factions_on) {
    return;
  }
  text << std::endl << std::endl << "<c=ltgray>On another band, voices " <<
          "argue about the Lab.  " << lab_race(this) << "<c=/>";
}

/**** Endings ****/

void Game::faction_victory(int f)
{
  std::stringstream text;
  std::string name = player->get_name();
  switch (f) {
    case GROUP_CULTISTS:
      text << "<c=pink>The garden blooms.<c=/>\n\nThe Lab's lattice " <<
        "turns, and the seams open wide.  Fungus climbs out from the centre " <<
        "of the island in every direction, and the aliens walk among the " <<
        "faithful as friends.  When the island is covered, the great portal " <<
        "will open, and the Second Coming will be complete.\n\n" << name <<
        " stands at the hierophant's side, changed, and glad of it.";
      break;
    case GROUP_MILITARY:
      text << "<c=ltgreen>The island is under orders.<c=/>\n\nThe Military " <<
        "holds the Lab, and its engineers bend the seams to their will: " <<
        "storms of twisted space they can aim at any square on the map.  " <<
        "The camps pay their dues; the other factions are gone.  The island " <<
        "is a stepping stone now, and the mainland is next.\n\n" << name <<
        " wears an officer's stripes.";
      break;
    case GROUP_NATURALISTS:
      text << "<c=ltgreen>The island is free.<c=/>\n\nThe Naturalists hold the " <<
        "Lab, and vote to starve the seams of power.  The aliens come less " <<
        "often, then rarely, then not at all; the connection beyond is " <<
        "severed.  Nobody rules the island.  Everybody farms it.\n\n" << name <<
        " has a vote, a field, and a long table to eat at.";
      break;
    case GROUP_SCIENTISTS:
      text << "<c=ltblue>The data is ours.<c=/>\n\nThe Scientists hold the " <<
        "Lab and open its files: plans for weapons nobody has seen, " <<
        "satellites that answer again, and a way to step through a seam and " <<
        "come out precisely where you meant to.  The other factions find " <<
        "that knowledge is power, too late.\n\n" << name << " has a name on a " <<
        "door in the station.";
      break;
  }
  int days = (time.get_turn() - Time(0, 0, 8, 1, SEASON_SPRING,
                                     STARTING_YEAR).get_turn()) / DAYS(1);
  text << "\n\n" << name << " survived " << days_text(days) << ".";
  popup(wrap_tagged(text.str(), 66).c_str());
  delete_save();
  game_over = true;
  player->action_points = 0;
}

void Game::lab_lost(int f)
{
  std::stringstream text;
  std::string who = GROUP_NAME[f];
  who[0] = toupper(who[0]);
  const char* fate[NUM_GROUPS] = { "", "",
    "Fungus spreads out from the Lab in every direction, and the faithful \
sing as the aliens walk among them.",
    "Soldiers patrol every road, and the Lab's seams are aimed like guns.",
    "The Naturalists starve the seams, and vote on what to do about you.",
    "The Scientists' eyebots watch every square of the island from above." };
  text << "<c=ltred>" << who << " hold the Lab for good.<c=/>\n\n" <<
    "Their days of holding it pass.  " << fate[f] << "  The island is " <<
    "theirs, and there is no place on it for someone who stood against " <<
    "them.\n\nThe game is over.";
  popup(wrap_tagged(text.str(), 66).c_str());
  delete_save();
  game_over = true;
  player->action_points = 0;
}
