#ifndef _GAME_H_
#define _GAME_H_

#include "map.h"
#include "cuss.h"
#include "worldmap.h"
#include "monster.h"
#include "player.h"
#include "keybind.h"
#include "pathfind.h"
#include "time.h"
#include "sound.h"
#include <sstream>
#include <vector>

// Identical messages within MESSAGE_GAP turns of each other are combined
#define MESSAGE_GAP 3

// The wreck of the player's boat, and what has been done to it (escape.cpp)
struct Boat_project
{
  Boat_project() { hull = false; sail = false; gps = false;
                   rations = 0; drinks = 0; }
  bool hull, sail, gps;
  int rations, drinks;
};

std::string days_text(int days);   // "3 days" (escape.cpp)

// The Lab and what powers it (lab.cpp)
struct Lab_state
{
  Lab_state() { plant_on = false; substation_on = false; open = false;
                control_on = false; corner_power = 0; holder = -1;
                countdown = 0; }
  bool plant_on, substation_on, open;
  bool control_on;    // the Control Station's remote switch is thrown
  int corner_power;   // corner Substations switched on (bits 0 to 2)
  int holder;         // the faction holding the Lab (GROUP_*), or -1
  int countdown;      // the turn their hold becomes victory
};

// People and factions (people.cpp), after Whales' notes (docs/notes.txt).
enum Group_id
{
  GROUP_NONE = -1,
  GROUP_SURVIVORS = 0,
  GROUP_RAIDERS,
  GROUP_CULTISTS,
  GROUP_MILITARY,
  GROUP_NATURALISTS,
  GROUP_SCIENTISTS,
  NUM_GROUPS
};

struct Faction_state
{
  Faction_state() { standing = 0; missions = 0; progress = 0; }
  int standing;  // what they think of you
  int missions;  // errands done for them
  int progress;  // how near they are to taking the Lab (0 to 100)
};

enum Job_kind
{
  JOB_NONE = 0,
  JOB_SCOUT,    // go and look at a place
  JOB_CLEAR,    // clear a place of what threatens them
  JOB_FETCH,    // fetch a supply case left at a place
  JOB_DELIVER,  // carry a sealed letter to another group
  JOB_POWER     // switch a corner Substation on
};

struct Job
{
  Job() { giver = GROUP_NONE; kind = JOB_NONE; to = GROUP_NONE;
          reward = 0; placed = false; done = false; bit = 0; }
  bool active() const { return kind != JOB_NONE; }
  int giver, kind, to, reward, bit;
  Point target;        // world map square
  std::string place;   // "the raider camp", "the bunker" ...
  bool placed;         // the case is on the ground
  bool done;           // done: report back to the giver's people
};

// Something hidden at a world map square, placed when the square is
// first in view (a passcard in the largest town, a case for an errand).
struct Planted_item
{
  Point world;
  std::string item;
  bool placed;
};

struct Game_message
{
  std::string text;
  int turn;
  int count;
  Game_message() { turn = 0; count = 1; text = ""; }
  Game_message(std::string T, int TN) : text (T), turn (TN) { count = 1; }
};

class Game
{
public:
  Game();
  ~Game();

/**** Setup - Called only once ****/
  bool setup_ui();
  bool starting_menu(); // Choose from create character, load character, etc.
/* setup_new_game() creates the Worldmap, Map, and Player objects.
 * If a world_index is passed to it, it'll attempt to load that Worldmap - the
 * index refers to an element in worldmap_names.  Otherwise, it'll create a new
 * Worldmap.
 */
  bool setup_new_game(int world_index = -1);
  int  world_screen(); // Returns a new world_index
  void create_world();

/**** Saving - savegame.cpp ****/
  std::string save_file_for(std::string world_name);
  bool has_save(std::string world_name);
  bool save_game();
  void delete_save();
  bool load_game(std::string world_name);
  std::string pick_saved_game();  // "" if cancelled
  bool player_is_dead();
/**** Survival - survival.cpp ****/
  bool danger_near();
  void interrupt_activity(std::string why = ".");
  void start_sleeping();
  bool water_interaction(Tripoint pos);
/**** The way off the island - escape.cpp ****/
  bool boat_ready();
  void boat_interface();
  void finish_boat_work(int stage);
  void listen_radio();
  void read_gps();
  void victory();
  Boat_project boat;
/**** The Lab - lab.cpp ****/
  bool examine_lab_machinery(Tripoint pos);
  void finish_lab_work(int stage);
  void lab_victory();
  Lab_state lab;
/**** People and factions - people.cpp ****/
  void talk_to(Monster* mon);
  void people_turn();          // errands, hidden things, the factions' clock
  void faction_dawn();         // a day of the factions' race for the Lab
  void faction_news(std::stringstream& text);  // for the radio
  bool group_hostile_to_player(int group);
  void hurt_standing(int group, int amount);
  void faction_victory(int group);
  void lab_lost(int group);
  std::string job_text();
  Faction_state factions[NUM_GROUPS];
  int member;                  // the faction you joined, or GROUP_NONE
  Job job;
  std::vector<Planted_item> planted;
  bool sandbox;                // no faction clock, no countdowns
  bool factions_on;
/**** Survivor camps - camp.cpp ****/
  void trade_interface();
  int trade_credit;
  std::string camp_job;   // what the camps asked for, or ""
  void player_death();

/**** Engine - Main loop functions ****/
  bool main_loop();
  void reset_temp_values();
  void do_action(Interface_action act);
  void move_entities();
  void clean_up_dead_entities();
  void handle_player_activity();
  void complete_player_activity();
  void process_active_items();

/**** Engine - Called-as-needed ****/
  void shift_if_needed();  // Shift the map, if the player's not in the center
  void return_offmap_monsters();
  void make_sound(std::string desc, int volume, Tripoint pos);
  void make_sound(std::string desc, int volume, Point pos);
  void make_sound(std::string desc, int volume, int x, int y);
  void make_sound(Sound snd, Tripoint pos);

// Pass NULL as shooter and Item() as it
  void launch_projectile(Ranged_attack attack,
                         Tripoint origin, Tripoint target);
// Pass NULL as shooter
  void launch_projectile(Item it, Ranged_attack attack,
                         Tripoint origin, Tripoint target);
// Pass Item() as it
  void launch_projectile(Entity* shooter, Ranged_attack attack,
                         Tripoint origin, Tripoint target);
// This one is the *real* one
  void launch_projectile(Entity* shooter, Item it, Ranged_attack attack,
                         Tripoint origin, Tripoint target);

  void player_move(int xdif, int ydif); // Handles all aspects of moving player
  void player_move_vertical(int zdif);

  void add_msg(std::string msg, ...);
// msg_query_yn adds a message, refreshes the HUD, and accepts Y/N input
  bool msg_query_yn(std::string msg, ...);

  void add_active_item(Item* it);
  void remove_active_item(Item* it);
  void remove_active_item_uid(int uid);
  bool destroy_item(Item* it, int uid = -1);
  bool destroy_item_uid(int uid); // destroy_item(NULL, uid)
// Temp value mutators
  void set_temp_light_level(int level);

/**** UI - Output functions ****/
  void draw_all();
  void update_hud();
  void print_messages();

/**** UI - Special screens and inputs ****/
  void debug_command();
  void pickup_items(Tripoint pos);
  void pickup_items(Point    pos);
  void pickup_items(int posx, int posy);
// TODO: Both are limited in that they can not return a point that the player
//       cannot currently see (they return Tripoint() instead).
  Tripoint target_selector(int startx = -1, int starty = -1,
                           int range  = -1, bool target_entites = false,
                           bool show_path = false);
  std::vector<Tripoint> path_selector(int startx = -1, int starty = -1,
                                      int range  = -1,
                                      bool target_entities = false,
                                      bool show_path = true);

/**** Data - Universal access functions ****/
  int get_item_uid();     // Get the next available UID (and increment)
  int get_furniture_uid();// Get the next available UID (and increment)
  bool minute_timer(int minutes); // Returns true once every $minutes minutes
  bool turn_timer(int turns);     // Returns true once every $turns turns
  int get_light_level();          // Current light distance, based on the time

/* is_empty() returns true if and only if the specified tile has a move_cost > 0
 * and there are no entities there.
 * TODO: Other requirements?
 */
  bool is_empty(int x, int y, int z);
  bool is_empty(Tripoint pos);

/* find_item() returns the location of the item.  If it == NULL, use the uid;
 * otherwise, use it.  If it == NULL and uid == -1, just fail immediately.
 * Returns [-1, -1, -1] on fail.
 */
  Tripoint find_item(Item* it, int uid = -1);
  Tripoint find_item_uid(int uid);  // find_item(NULL, uid)

/**** Contained data ****/
  Map*        map;
  Worldmap*   worldmap;
// Note that player should always == &(entities[0])
  Player*     player;
  Entity_pool entities;

  Time time;

// Not used; TODO: Remove this?
  Generic_map scent_map;

private:
  Window *w_map;
  Window *w_hud;
  cuss::interface i_hud;
  std::vector<Game_message> messages;
  std::vector<Item*> active_items;
  std::vector<std::string> worldmap_names;
  int last_target;
  int new_messages;
  int next_item_uid;
  int next_furniture_uid;
  bool game_over;

// Temp values; all reset in reset_temp_values()
  int temp_light_level;

// Just adds the message, after it's been formatted and everything.
// Used by add_msg(), msg_query_yn(), and any other similar functions.
  void add_msg_static(std::string message);
};

#endif
