// camp.cpp: survivor camps that trade (Asciiverse, 2026).
//
// Whales planned "several smaller camps of survivors, some of which will be
// happy to trade their farm-grown food with the player"
// (https://whalesdev.tumblr.com/post/74991567472), with food "bought from
// NPCs ... by acquiring high-value loot"
// (https://whalesdev.tumblr.com/post/91112281541).  There are no NPCs in
// the engine yet; a camp's trader stands behind the trade counter in its
// map, and the prices are in data/trade.dat.

#include "game.h"
#include "worldmap.h"
#include "player.h"
#include "globals.h"
#include "files.h"
#include "window.h"
#include "stringfunc.h"
#include "rng.h"
#include <fstream>
#include <sstream>
#include <map>

std::string wrap_tagged(std::string text, int width);           // escape.cpp
int text_menu(std::string text, std::vector<std::string> options);

/**** World generation: three camps on the main island ****/

void Worldmap::place_camps()
{
  if (islands.count(0) == 0) {
    return;
  }
  std::vector<Point> spots;
  std::vector<Point>& land = islands[0];
  for (int i = 0; i < land.size(); i++) {
    Worldmap_tile* tile = get_tile(land[i]);
    if (tile && tile->terrain && (tile->terrain->name == "field" ||
                                  tile->terrain->name == "forest")) {
      spots.push_back(land[i]);
    }
  }
  std::vector<Point> placed;
  for (int tries = 0; placed.size() < 3 && tries < 400 && !spots.empty();
       tries++) {
    Point p = spots[rng(0, spots.size() - 1)];
    bool ok = true;
// Away from each other and from the Lab's grounds.
    for (int i = 0; i < placed.size(); i++) {
      if (rl_dist(p, placed[i]) < 15) {
        ok = false;
      }
    }
    for (int dx = -4; ok && dx <= 4; dx++) {
      for (int dy = -4; ok && dy <= 4; dy++) {
        Worldmap_tile* near = get_tile(p.x + dx, p.y + dy);
        if (near && near->terrain &&
            (near->terrain->name == "lab" ||
             near->terrain->name == "power plant" ||
             near->terrain->name == "lab substation")) {
          ok = false;
        }
      }
    }
    if (ok) {
      tiles[p.x][p.y].set_terrain("survivor camp");
      tiles[p.x][p.y].monsters.clear();   // they keep their ground clear
      placed.push_back(p);
    }
  }
}

/**** The trade table ****/

struct Trade_table
{
  bool loaded;
  std::map<std::string,int> buys;   // what a camp gives for one
  std::vector<std::string> sell_order;
  std::map<std::string,int> sells;  // what one costs
  Trade_table() { loaded = false; }
};

static Trade_table TRADE;

static void load_trade_table()
{
  if (TRADE.loaded) {
    return;
  }
  TRADE.loaded = true;
  std::ifstream fin( (DATA_DIR + "/trade.dat").c_str() );
  std::string line;
  while (std::getline(fin, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') {
      continue;
    }
    size_t colon = line.find(':'), eq = line.rfind('=');
    if (colon == std::string::npos || eq == std::string::npos || eq < colon) {
      continue;
    }
    std::string kind = no_caps(trim(line.substr(0, colon)));
    std::string name = no_caps(trim(line.substr(colon + 1, eq - colon - 1)));
    int value = atoi(line.substr(eq + 1).c_str());
    if (!ITEM_TYPES.lookup_name(name) || value <= 0) {
      debugmsg("trade.dat: unknown item or price in '%s'", line.c_str());
      continue;
    }
    if (kind == "buys") {
      TRADE.buys[name] = value;
    } else if (kind == "sells") {
      TRADE.sells[name] = value;
      TRADE.sell_order.push_back(name);
    }
  }
}

/**** The trader ****/

static std::string camp_name(Point world)
{
  const char* names[6] = { "Hollow Farm", "Saltwater Camp", "Mill Creek",
                           "the Stockade", "Greywater", "Thistle Hill" };
  return names[(world.x * 7 + world.y * 13) % 6];
}

static std::string trader_name(Point world)
{
  const char* names[6] = { "Marta", "Old Tobias", "Ines", "Ruben",
                           "Grace", "Hal" };
  return names[(world.x * 11 + world.y * 5) % 6];
}

void Game::trade_interface()
{
  load_trade_table();
  Point world(map->posx + MAP_SIZE / 2, map->posy + MAP_SIZE / 2);
  std::string camp = camp_name(world), trader = trader_name(world);

  while (true) {
    std::stringstream text;
    text << "<c=white>" << trader << ", at the trade counter of " << camp <<
            ".<c=/>" << std::endl << std::endl;
    text << "\"Money's mostly paper now, but folk still want it, and " <<
            "anything that keeps you alive.  You've got <c=yellow>" <<
            trade_credit << "<c=/> on account with us.\"";
    std::vector<std::string> options;
    options.push_back("Sell them something");
    options.push_back("Buy from them");
    options.push_back("Leave");
    int choice = text_menu(text.str(), options);

    if (choice == 0) {
// What we carry that they take, one line per kind.
      std::vector<std::string> kinds;
      std::vector<int> values;
      std::vector<std::string> labels;
      for (int i = 0; i < player->inventory.size(); i++) {
        Item& it = player->inventory[i];
        if (!it.is_real()) {
          continue;
        }
        std::string name = no_caps(it.get_type()->name);
        if (TRADE.buys.count(name) == 0) {
          continue;
        }
        bool seen = false;
        for (int k = 0; k < kinds.size(); k++) {
          if (kinds[k] == name) {
            seen = true;
          }
        }
        if (!seen && kinds.size() < 18) {
          kinds.push_back(name);
          values.push_back(TRADE.buys[name]);
          std::stringstream lab;
          lab << it.get_type()->name << "  <c=yellow>+" << TRADE.buys[name] <<
                 "<c=/>";
          labels.push_back(lab.str());
        }
      }
      if (kinds.empty()) {
        popup(wrap_tagged("\"Nothing there we need.  Money, jewellery, \
medicine, batteries, bullets, good liquor, the insides of computers: \
that's what we take.\"", 70).c_str());
        continue;
      }
      labels.push_back("Never mind");
      int pick = text_menu("\"What have you got?\"  One at a time.", labels);
      if (pick < 0 || pick >= kinds.size()) {
        continue;
      }
      for (int i = 0; i < player->inventory.size(); i++) {
        Item& it = player->inventory[i];
        if (it.is_real() && no_caps(it.get_type()->name) == kinds[pick]) {
          if (it.count > 1) {
            it.count--;
          } else {
            player->inventory.erase(player->inventory.begin() + i);
          }
          trade_credit += values[pick];
          add_msg("You sell the %s for %d.", kinds[pick].c_str(),
                  values[pick]);
          break;
        }
      }

    } else if (choice == 1) {
      std::vector<std::string> labels;
      for (int i = 0; i < TRADE.sell_order.size(); i++) {
        std::string name = TRADE.sell_order[i];
        int price = TRADE.sells[name];
        std::stringstream lab;
        lab << (price <= trade_credit ? "" : "<c=dkgray>") <<
               ITEM_TYPES.lookup_name(name)->name << "  " << price <<
               (price <= trade_credit ? "" : "<c=/>");
        labels.push_back(lab.str());
      }
      labels.push_back("Never mind");
      std::stringstream ask;
      ask << "\"Here's what we can spare.\"  You have " << trade_credit <<
             " on account.";
      int pick = text_menu(ask.str(), labels);
      if (pick < 0 || pick >= TRADE.sell_order.size()) {
        continue;
      }
      std::string name = TRADE.sell_order[pick];
      int price = TRADE.sells[name];
      if (price > trade_credit) {
        popup("\"That'll be %d, and you've got %d.  Come back with more.\"",
              price, trade_credit);
        continue;
      }
      Item bought( ITEM_TYPES.lookup_name(name) );
      bought.prep_for_generation();   // water comes in its bottle
      trade_credit -= price;
      if (!player->add_item(bought)) {
        map->add_item(bought, player->pos);
      }
      add_msg("You buy %s.", bought.get_name_indefinite().c_str());

    } else {
      return;
    }
  }
}
