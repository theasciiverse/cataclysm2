// escape.cpp: the way off the island (Asciiverse, 2026).
//
// Whales planned a "bad win" for players who join no faction: "construct or
// buy a boat powerful enough to reach the mainland", "pulling together their
// resources to both create a boat that will survive the high seas, and to
// repair and connect to a GPS network"
// (https://whalesdev.tumblr.com/post/86073588301,
//  https://whalesdev.tumblr.com/post/74991567472).  The wreck of the
// player's own boat lies on the starting beach; the radio from the wreck
// picks up a mainland relay that a GPS unit can follow.  The details (what
// the boat needs, the relay at Saltcove) are ours; see VISION.md.

#include "game.h"
#include "player.h"
#include "tool.h"
#include "globals.h"
#include "window.h"
#include "stringfunc.h"
#include "rng.h"
#include <sstream>

// What the boat needs.
#define BOAT_PLANKS      10
#define BOAT_NAILS       60
#define BOAT_TARPS        2
#define BOAT_ROPES        2
#define BOAT_POLES        1
#define BOAT_RATIONS      8   // food or drink, of which ...
#define BOAT_DRINKS       4   // ... at least this many to drink


/**** Text helpers ****/

// Wraps text to width columns; <c=...> colour tags take no room.
std::string wrap_tagged(std::string text, int width)
{
  std::string ret, word;
  int col = 0, wordlen = 0;
  for (size_t i = 0; i <= text.size(); i++) {
    char c = (i < text.size() ? text[i] : '\n');
    if (c == '<' && i + 2 < text.size() && text[i + 1] == 'c' &&
        text[i + 2] == '=') {
      size_t end = text.find('>', i);
      if (end != std::string::npos) {
        word += text.substr(i, end - i + 1);
        i = end;
        continue;
      }
    }
    if (c == ' ' || c == '\n') {
      if (col > 0 && col + wordlen > width) {
        ret += "\n";
        col = 0;
      }
      ret += word;
      col += wordlen;
      word = "";
      wordlen = 0;
      if (c == '\n') {
        if (i < text.size()) {
          ret += "\n";
        }
        col = 0;
      } else if (col < width) {
        ret += " ";
        col++;
      }
    } else {
      word += c;
      wordlen++;
    }
  }
  return ret;
}

// One window with the text above and lettered choices below; returns the
// index chosen, or -1 for Esc.
static int text_menu(std::string text, std::vector<std::string> options)
{
  std::vector<std::string> lines;
  std::string wrapped = wrap_tagged(text, 72);
  size_t start = 0, pos;
  while ((pos = wrapped.find('\n', start)) != std::string::npos) {
    lines.push_back(wrapped.substr(start, pos - start));
    start = pos + 1;
  }
  lines.push_back(wrapped.substr(start));
  int height = lines.size() + options.size() + 3;
  if (height > 24) {
    height = 24;
  }
  Window w(2, (24 - height) / 2, 76, height);
  w.outline();
  int y = 1;
  for (int i = 0; i < lines.size() && y < height - 1; i++, y++) {
    w.putstr(2, y, c_ltgray, c_black, lines[i]);
  }
  y++;
  for (int i = 0; i < options.size() && y < height - 1; i++, y++) {
    w.putstr(2, y, c_white, c_black, "<c=yellow>%c<c=/>: %s", 'a' + i,
             options[i].c_str());
  }
  w.refresh();
  while (true) {
    long ch = getch();
    if (ch == KEY_ESC || ch == 'q' || ch == ' ') {
      return -1;
    }
    if (ch >= 'a' && ch < 'a' + int(options.size())) {
      return ch - 'a';
    }
  }
}

std::string days_text(int days)
{
  if (days <= 0) {
    return "less than a day";
  }
  std::stringstream ret;
  ret << days << (days == 1 ? " day" : " days");
  return ret.str();
}

/**** Tool specials ****/

static bool skip_to_done(std::istream& data)
{
  std::string ident;
  while (data >> ident) {
    if (no_caps(ident) == "done") {
      return true;
    }
  }
  return false;
}

bool Tool_special_radio::load_data(std::istream& data, std::string owner_name)
{
  return skip_to_done(data);
}

bool Tool_special_radio::effect(Entity* user)
{
  if (user && user->is_you()) {
    GAME.listen_radio();
  }
  return true;
}

bool Tool_special_gps::load_data(std::istream& data, std::string owner_name)
{
  return skip_to_done(data);
}

bool Tool_special_gps::effect(Entity* user)
{
  if (user && user->is_you()) {
    GAME.read_gps();
  }
  return true;
}

/**** Finding materials: carried, or lying within two tiles of us ****/

struct Material_source
{
  std::vector<Item>* list;
  int index;
};

static std::vector<Material_source> find_materials(Game* game,
                                                   std::string name)
{
  std::vector<Material_source> ret;
  Player* pl = game->player;
  name = no_caps(name);
  for (int i = 0; i < pl->inventory.size(); i++) {
    if (pl->inventory[i].is_real() &&
        no_caps(pl->inventory[i].get_type()->name) == name) {
      Material_source src = { &(pl->inventory), i };
      ret.push_back(src);
    }
  }
  for (int dx = -2; dx <= 2; dx++) {
    for (int dy = -2; dy <= 2; dy++) {
      std::vector<Item>* items = game->map->items_at(pl->pos.x + dx,
                                                     pl->pos.y + dy,
                                                     pl->pos.z);
      if (!items) {
        continue;
      }
      for (int i = 0; i < items->size(); i++) {
        if ((*items)[i].is_real() &&
            no_caps((*items)[i].get_type()->name) == name) {
          Material_source src = { items, i };
          ret.push_back(src);
        }
      }
    }
  }
  return ret;
}

// How much one item is worth: a box of nails holds its nails as charges,
// other stacks have a count.
static int amount_in(Item& it)
{
  if (it.get_item_class() == ITEM_CLASS_AMMO) {
    return (it.charges > 0 ? it.charges : 1);
  }
  return (it.count > 0 ? it.count : 1);
}

// Items are stacks: a box of nails has a count of 100.
static int count_material(Game* game, std::string name)
{
  std::vector<Material_source> src = find_materials(game, name);
  int total = 0;
  for (int i = 0; i < src.size(); i++) {
    total += amount_in((*src[i].list)[src[i].index]);
  }
  return total;
}

static bool has_tool(Game* game, std::string name)
{
  Player* pl = game->player;
  if (pl->weapon.is_real() && no_caps(pl->weapon.get_type()->name) ==
                              no_caps(name)) {
    return true;
  }
  return !find_materials(game, name).empty();
}

// Removes amount of name, from the pack first, then the ground.
static void use_material(Game* game, std::string name, int amount)
{
  while (amount > 0) {
    std::vector<Material_source> src = find_materials(game, name);
    if (src.empty()) {
      return;
    }
    Material_source s = src[0];
    Item* it = &((*s.list)[s.index]);
    int c = amount_in(*it);
    if (c > amount) {
      if (it->get_item_class() == ITEM_CLASS_AMMO) {
        it->charges = c - amount;
      } else {
        it->count = c - amount;
      }
      return;
    }
    amount -= c;
    s.list->erase(s.list->begin() + s.index);
  }
}

static std::string need_line(std::string what, int have, int need)
{
  std::stringstream ret;
  ret << (have >= need ? "<c=ltgreen>" : "<c=ltred>") << what << ": " <<
         (have > need ? need : have) << "/" << need << "<c=/>";
  return ret.str();
}

/**** Rations ****/

// A ration is one food item, or one container holding food or drink.
static bool is_ration(Item& it, bool& drink)
{
  Item* food = NULL;
  if (it.get_item_class() == ITEM_CLASS_FOOD) {
    food = &it;
  } else if (it.get_item_class() == ITEM_CLASS_CONTAINER &&
             !it.contents.empty() &&
             it.contents[0].get_item_class() == ITEM_CLASS_FOOD) {
    food = &(it.contents[0]);
  }
  if (!food) {
    return false;
  }
  Item_type_food* ft = static_cast<Item_type_food*>(food->get_type());
  drink = (ft->water > 0 && ft->food <= 0);
  return true;
}

/**** The boat ****/

bool Game::boat_ready()
{
  return boat.hull && boat.sail && boat.gps &&
         boat.rations >= BOAT_RATIONS && boat.drinks >= BOAT_DRINKS;
}

void Game::boat_interface()
{
  while (true) {
    int planks = count_material(this, "wood plank");
    int nails  = count_material(this, "nails");
    int tarps  = count_material(this, "tarp");
    int ropes  = count_material(this, "rope");
    int poles  = count_material(this, "metal pole");
    bool hammer = has_tool(this, "hammer");
    std::vector<Material_source> gps_units = find_materials(this, "GPS unit");

    std::stringstream info;
    info << "<c=white>The wreck of your boat.<c=/>" << std::endl << std::endl;

    if (boat.hull) {
      info << "<c=ltgreen>Hull: patched and sound.<c=/>" << std::endl;
    } else {
      info << "Hull: split along the keel.  " <<
              need_line("planks", planks, BOAT_PLANKS) << "  " <<
              need_line("nails", nails, BOAT_NAILS) << "  " <<
              (hammer ? "<c=ltgreen>hammer<c=/>" : "<c=ltred>a hammer<c=/>") <<
              std::endl;
    }
    if (boat.sail) {
      info << "<c=ltgreen>Mast and sail: rigged.<c=/>" << std::endl;
    } else {
      info << "Mast and sail: gone.  " <<
              need_line("tarps", tarps, BOAT_TARPS) << "  " <<
              need_line("rope", ropes, BOAT_ROPES) << "  " <<
              need_line("metal pole", poles, BOAT_POLES) << std::endl;
    }
    if (boat.gps) {
      info << "<c=ltgreen>Navigation: GPS unit mounted.<c=/>" << std::endl;
    } else {
      info << "Navigation: none.  " <<
              (gps_units.empty() ? "<c=ltred>a GPS unit<c=/>" :
                                   "<c=ltgreen>GPS unit<c=/>") << std::endl;
    }
    info << "Provisions: " <<
            need_line("rations", boat.rations, BOAT_RATIONS) << ", of them " <<
            need_line("to drink", boat.drinks, BOAT_DRINKS) << std::endl;
    info << std::endl << "<c=dkgray>Materials count if you carry them or they " <<
            "lie within two steps of you.<c=/>";

    std::vector<std::string> options;
    std::vector<int> actions;
    if (!boat.hull) {
      options.push_back("Patch the hull (6 hours)");
      actions.push_back(1);
    }
    if (!boat.sail) {
      options.push_back("Rig the mast and sail (3 hours)");
      actions.push_back(2);
    }
    if (!boat.gps) {
      options.push_back("Mount a GPS unit (30 minutes)");
      actions.push_back(3);
    }
    options.push_back("Stow food and drink you carry");
    actions.push_back(4);
    if (boat_ready()) {
      options.push_back("<c=yellow>Set sail for the mainland<c=/>");
      actions.push_back(5);
    }
    options.push_back("Leave it be");
    actions.push_back(0);

    int choice = text_menu(info.str(), options);
    if (choice < 0 || choice >= actions.size() || actions[choice] == 0) {
      return;
    }

    switch (actions[choice]) {
      case 1:
        if (planks < BOAT_PLANKS || nails < BOAT_NAILS || !hammer) {
          popup("You need %d wood planks, %d nails and a hammer.",
                BOAT_PLANKS, BOAT_NAILS);
          break;
        }
        add_msg("You set to work on the hull.");
        player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, HOURS(6) * 100, 1);
        return;

      case 2:
        if (tarps < BOAT_TARPS || ropes < BOAT_ROPES || poles < BOAT_POLES) {
          popup("You need %d tarps, %d ropes and a metal pole.",
                BOAT_TARPS, BOAT_ROPES);
          break;
        }
        add_msg("You set to work on the mast and sail.");
        player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, HOURS(3) * 100, 2);
        return;

      case 3: {
        bool charged = false;
        for (int i = 0; i < gps_units.size(); i++) {
          if ((*gps_units[i].list)[gps_units[i].index].charges > 0) {
            charged = true;
          }
        }
        if (gps_units.empty()) {
          popup("You need a GPS unit.");
        } else if (!charged) {
          popup("The GPS unit's batteries are flat.  Load some batteries \
into it first.");
        } else {
          add_msg("You set to work mounting the GPS unit.");
          player->set_activity(PLAYER_ACTIVITY_BOAT_WORK, MINUTES(30) * 100,
                               3);
          return;
        }
      } break;

      case 4: {
        int stowed = 0;
        for (int i = 0; i < player->inventory.size(); ) {
          bool drink = false;
          if (is_ration(player->inventory[i], drink)) {
            int n = player->inventory[i].count > 0 ?
                    player->inventory[i].count : 1;   // a stack of several
            boat.rations += n;
            if (drink) {
              boat.drinks += n;
            }
            stowed += n;
            player->inventory.erase(player->inventory.begin() + i);
          } else {
            i++;
          }
        }
        if (stowed == 0) {
          popup("You carry no food or drink to stow.");
        } else {
          add_msg("You stow %d ration%s aboard.", stowed,
                  stowed == 1 ? "" : "s");
        }
      } break;

      case 5:
        if (query_yn("Push off from the island for good?")) {
          victory();
          return;
        }
        break;
    }
  }
}

void Game::finish_boat_work(int stage)
{
  switch (stage) {
    case 1:
      if (count_material(this, "wood plank") < BOAT_PLANKS ||
          count_material(this, "nails") < BOAT_NAILS) {
        add_msg("You run out of materials before the hull is done.");
        return;
      }
      use_material(this, "wood plank", BOAT_PLANKS);
      use_material(this, "nails", BOAT_NAILS);
      boat.hull = true;
      add_msg("<c=ltgreen>The hull is patched.  She'll float.<c=/>");
      break;
    case 2:
      if (count_material(this, "tarp") < BOAT_TARPS ||
          count_material(this, "rope") < BOAT_ROPES ||
          count_material(this, "metal pole") < BOAT_POLES) {
        add_msg("You run out of materials before the mast is up.");
        return;
      }
      use_material(this, "tarp", BOAT_TARPS);
      use_material(this, "rope", BOAT_ROPES);
      use_material(this, "metal pole", BOAT_POLES);
      boat.sail = true;
      add_msg("<c=ltgreen>The mast is stepped and the sail bent on.<c=/>");
      break;
    case 3:
      if (find_materials(this, "GPS unit").empty()) {
        add_msg("You no longer have a GPS unit.");
        return;
      }
      use_material(this, "GPS unit", 1);
      boat.gps = true;
      add_msg("<c=ltgreen>The GPS unit is mounted and locked onto the \
Saltcove relay.<c=/>");
      break;
  }
  if (boat_ready()) {
    add_msg("<c=yellow>The boat is ready.  Examine it to set sail.<c=/>");
  }
}

/**** The radio and the GPS unit ****/

void Game::listen_radio()
{
  std::stringstream text;
  text << "<c=ltgray>*crackle*  ...is the Saltcove coastal relay, on the " <<
          "automatic loop.  Saltcove harbour is walled and held.  Boats are " <<
          "welcome.  Craft with a working GPS receiver can lock onto this " <<
          "relay and follow it to the harbour mouth.  Repeat: this is the " <<
          "Saltcove coastal relay...  *crackle*<c=/>" << std::endl << std::endl;
  text << "<c=ltgray>Further down the dial, faint, a recorded voice: " <<
          "\"...Central Research island facility.  Main power lost.  " <<
          "Containment at the core cannot be guaranteed.  Staff holding " <<
          "passcards, restore power from the plant and the Lab substation " <<
          "and secure the core...\"<c=/>" << std::endl << std::endl;
  if (lab.open) {
    text << "The Lab stands open at the island's heart.  Its core is " <<
            "where the monsters come from." << std::endl << std::endl;
  } else {
    text << "A lab at the island's heart, with a door that wants power and " <<
            "a passcard." << std::endl << std::endl;
  }
  if (boat_ready()) {
    text << "Your boat is ready on the beach.  All that's left is to sail.";
  } else {
    text << "Saltcove.  The mainland.  If the wreck on the beach could be " <<
            "made to float again, with a sail and a GPS unit to follow that " <<
            "signal, you could leave this island behind.  Still needed:";
    if (!boat.hull) {
      text << std::endl << "  - the hull patched: " << BOAT_PLANKS <<
              " wood planks, " << BOAT_NAILS << " nails, and a hammer";
    }
    if (!boat.sail) {
      text << std::endl << "  - a mast and sail: " << BOAT_TARPS <<
              " tarps, " << BOAT_ROPES << " ropes, a metal pole";
    }
    if (!boat.gps) {
      text << std::endl << "  - a GPS unit with batteries";
    }
    if (boat.rations < BOAT_RATIONS || boat.drinks < BOAT_DRINKS) {
      text << std::endl << "  - food and drink for the crossing (" <<
              BOAT_RATIONS << " rations, " << BOAT_DRINKS << " to drink)";
    }
  }
  popup(wrap_tagged(text.str(), 74).c_str());
}

void Game::read_gps()
{
  popup(wrap_tagged("The GPS unit searches the sky and finds nothing: the satellites \
fell silent long ago.  Then, on the ground band, it locks onto a faint \
signal: the Saltcove relay, somewhere west across the water.\n\n\
Mounted on a seaworthy boat, it could lead you there.", 74).c_str());
}

/**** Victory ****/

void Game::victory()
{
  int days = (time.get_turn() - Time(0, 0, 8, 1, SEASON_SPRING,
                                     STARTING_YEAR).get_turn()) / DAYS(1);
  std::string name = player->get_name();
  std::stringstream text;
  text << "<c=ltgreen>You set sail.<c=/>\n\n" <<
    "The patched hull takes the first waves well.  The island falls behind " <<
    "you, its towns and woods and everything in them, until it is a dark " <<
    "line, and then nothing.  The GPS unit's arrow holds steady on the " <<
    "Saltcove relay.\n\n" <<
    "Days later, gaunt and salt-burned, " << name << " sails between the " <<
    "harbour walls of Saltcove.\n\n" <<
    name << " survived " << days_text(days) << " on the island and escaped " <<
    "it.\n\n" <<
    "<c=dkgray>Whales called this the win of last resort: the island's " <<
    "factions and its Lab are still out there.<c=/>";
  popup(wrap_tagged(text.str(), 66).c_str());
  delete_save();
  game_over = true;
  player->action_points = 0;
}
