// savegame.cpp: saving and loading a game in progress (Asciiverse, 2026).
//
// Whales wrote the world's side of saving (submaps go to disk as the player
// moves) but never the player's.  A game is saved to one file per world,
// SAVE_DIR/<world>/game.sav, beside that world's map files:
//
//   Cataclysm2_save 1
//   Turn: <game turn>
//   Uids: <next item uid> <next furniture uid>
//   Map: <map posx> <map posy> <map posz>
//   Player: <Player::save_data()>
//   Monster: <Monster::save_data()>     (one per monster on the map)
//   Message: <text>                     (the last few messages)
//   Done
//
// Entity positions are the map's local coordinates, as in the game.

#include "game.h"
#include "entity.h"
#include "player.h"
#include "monster.h"
#include "skill.h"
#include "mission.h"
#include "status_effect.h"
#include "profession.h"
#include "globals.h"
#include "files.h"
#include "stringfunc.h"
#include "window.h"
#include <fstream>
#include <sstream>
#include <cstdio>

#define SAVE_FORMAT 1

// Reads the rest of the line, without the space after the key.
static std::string rest_of_line(std::istream& data)
{
  std::string line;
  std::getline(data, line);
  return trim(line);
}

/**** Skill_set ****/

std::string Skill_set::save_data()
{
  std::stringstream ret;
  ret << SKILL_MAX;
  for (int i = 0; i < SKILL_MAX; i++) {
    ret << " " << level[i] << " " << max_level[i] << " " << unlocked[i];
  }
  return ret.str();
}

bool Skill_set::load_data(std::istream& data)
{
  int count;
  data >> count;
  for (int i = 0; i < count; i++) {
    int lev, max, unl;
    data >> lev >> max >> unl;
    if (i < SKILL_MAX) {
      level[i] = lev;
      max_level[i] = max;
      unlocked[i] = (unl != 0);
    }
  }
  return !data.fail();
}

/**** Entity ****/

std::string Entity::save_data()
{
  std::stringstream ret;
  ret << "Pos: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
  ret << "Stats: " << stats.strength << " " << stats.dexterity << " " <<
         stats.intelligence << " " << stats.perception << " " <<
         stats.strength_max << " " << stats.dexterity_max << " " <<
         stats.intelligence_max << " " << stats.perception_max << std::endl;
  ret << "Skills: " << skills.save_data() << std::endl;
  ret << "Traits: " << traits.size();
  for (int i = 0; i < traits.size(); i++) {
    ret << " " << (traits[i] ? 1 : 0);
  }
  ret << std::endl;
  ret << "Experience: " << experience << std::endl;
  ret << "Needs: " << hunger << " " << thirst << " " << fatigue << " " <<
         stomach_food << " " << stomach_water << " " << pain << " " <<
         painkill << " " << special_timer << " " << summons_used << std::endl;
  ret << "Morale: " << morale << std::endl;
  for (int i = 0; i < missions.size(); i++) {
    Mission* mis = &(missions[i]);
    ret << "Mission: " << int(mis->type) << " " <<
           mis->original_target_count << " " << mis->target_count << " " <<
           mis->xp << " " << mis->deadline.get_turn() << " " <<
           (mis->personal ? 1 : 0) << " " << int(mis->status) << " " <<
           mis->target_name << std::endl;
  }
  for (int i = 0; i < effects.size(); i++) {
    ret << "Effect: " << int(effects[i].type) << " " << effects[i].duration <<
           " " << effects[i].level << std::endl;
  }
  for (std::map<std::string,int>::iterator it = chapters_read.begin();
       it != chapters_read.end();
       it++) {
    ret << "Chapters: " << it->second << " " << it->first << std::endl;
  }
  if (weapon.is_real()) {
    ret << "Weapon:" << std::endl << weapon.save_data() << std::endl;
  }
  for (int i = 0; i < inventory.size(); i++) {
    ret << "Carried:" << std::endl << inventory[i].save_data() << std::endl;
  }
  for (int i = 0; i < items_worn.size(); i++) {
    ret << "Worn:" << std::endl << items_worn[i].save_data() << std::endl;
  }
  return ret.str();
}

bool Entity::load_entity_field(const std::string& ident, std::istream& data)
{
  if (ident == "pos:") {
    data >> pos.x >> pos.y >> pos.z;

  } else if (ident == "stats:") {
    data >> stats.strength >> stats.dexterity >> stats.intelligence >>
            stats.perception >> stats.strength_max >> stats.dexterity_max >>
            stats.intelligence_max >> stats.perception_max;

  } else if (ident == "skills:") {
    skills.load_data(data);

  } else if (ident == "traits:") {
    int count;
    data >> count;
    for (int i = 0; i < count; i++) {
      int t;
      data >> t;
      if (i < traits.size()) {
        traits[i] = (t != 0);
      } else {
        traits.push_back(t != 0);
      }
    }

  } else if (ident == "experience:") {
    data >> experience;

  } else if (ident == "needs:") {
    data >> hunger >> thirst >> fatigue >> stomach_food >> stomach_water >>
            pain >> painkill >> special_timer >> summons_used;

  } else if (ident == "morale:") {
    data >> morale;

  } else if (ident == "mission:") {
    Mission mis;
    int type, deadline, personal, status;
    data >> type >> mis.original_target_count >> mis.target_count >> mis.xp >>
            deadline >> personal >> status;
    mis.type = Mission_type(type);
    mis.deadline = Time(deadline);
    mis.personal = (personal != 0);
    mis.status = Mission_status(status);
    mis.target_name = rest_of_line(data);
    missions.push_back(mis);
    return !data.fail();

  } else if (ident == "effect:") {
    int type, duration, level;
    data >> type >> duration >> level;
    if (type > STATUS_NULL && type < STATUS_MAX) {
      effects.push_back( Status_effect(Status_effect_type(type), duration,
                                       level) );
    }

  } else if (ident == "chapters:") {
    int count;
    data >> count;
    std::string title = rest_of_line(data);
    chapters_read[title] = count;
    return !data.fail();

  } else if (ident == "weapon:") {
    Item tmp;
    if (!tmp.load_data(data)) {
      return false;
    }
    weapon = tmp;
    return true;

  } else if (ident == "carried:") {
    Item tmp;
    if (!tmp.load_data(data)) {
      return false;
    }
    inventory.push_back(tmp);
    return true;

  } else if (ident == "worn:") {
    Item tmp;
    if (!tmp.load_data(data)) {
      return false;
    }
    items_worn.push_back(tmp);
    return true;

  } else {
    debugmsg("Unknown saved entity field '%s'.", ident.c_str());
    return false;
  }
  std::string junk;
  std::getline(data, junk);
  return !data.fail();
}

bool Entity::load_data(std::istream& data)
{
  std::string ident;
  while (data >> ident) {
    ident = no_caps(ident);
    if (ident == "done") {
      return true;
    }
    if (!load_entity_field(ident, data)) {
      return false;
    }
  }
  return false;
}

/**** Player ****/

std::string Player::save_data()
{
  std::stringstream ret;
  ret << "Name: " << name << std::endl;
  ret << "Male: " << (male ? 1 : 0) << std::endl;
  if (profession) {
    ret << "Profession: " << profession->name << std::endl;
  }
  ret << "HP: " << int(HP_PART_MAX);
  for (int i = 0; i < HP_PART_MAX; i++) {
    ret << " " << current_hp[i] << " " << max_hp[i];
  }
  ret << std::endl;
  for (std::map<std::string,int>::iterator it = chapters_read.begin();
       it != chapters_read.end();
       it++) {
    ret << "Read: " << it->second << " " << it->first << std::endl;
  }
  ret << Entity::save_data();
  ret << "Done";
  return ret.str();
}

bool Player::load_data(std::istream& data)
{
  std::string ident;
  while (data >> ident) {
    ident = no_caps(ident);
    if (ident == "done") {
      return true;

    } else if (ident == "name:") {
      name = rest_of_line(data);

    } else if (ident == "male:") {
      int m;
      data >> m;
      male = (m != 0);

    } else if (ident == "profession:") {
      std::string prof = rest_of_line(data);
      profession = PROFESSIONS.lookup_name(prof);

    } else if (ident == "hp:") {
      int count;
      data >> count;
      for (int i = 0; i < count; i++) {
        int cur, max;
        data >> cur >> max;
        if (i < HP_PART_MAX) {
          current_hp[i] = cur;
          max_hp[i] = max;
        }
      }

    } else if (ident == "read:") {
      int count;
      data >> count;
      chapters_read[ rest_of_line(data) ] = count;

    } else if (!load_entity_field(ident, data)) {
      return false;
    }
  }
  return false;
}

/**** Monster ****/

std::string Monster::save_data()
{
  std::stringstream ret;
  if (type) {
    ret << "Type: " << type->name << std::endl;
  }
  ret << "MonHP: " << current_hp << " " << max_hp << std::endl;
  ret << "Pos: " << pos.x << " " << pos.y << " " << pos.z << std::endl;
  ret << "Done";
  return ret.str();
}

bool Monster::load_data(std::istream& data)
{
  std::string ident;
  while (data >> ident) {
    ident = no_caps(ident);
    if (ident == "done") {
      return type != NULL;

    } else if (ident == "type:") {
      std::string tname = rest_of_line(data);
      Monster_type* mt = MONSTER_TYPES.lookup_name(tname);
      if (!mt) {
        debugmsg("Unknown saved monster type '%s'.", tname.c_str());
        return false;
      }
      set_type(mt);

    } else if (ident == "monhp:") {
      data >> current_hp >> max_hp;

    } else if (!load_entity_field(ident, data)) {
      return false;
    }
  }
  return false;
}

/**** Submap_pool ****/

void Submap_pool::save_all()
{
  if (!GAME.worldmap) {
    return;
  }
  std::string map_dir = SAVE_DIR + "/" + GAME.worldmap->get_name();
  if (!directory_exists(map_dir) && !create_directory(map_dir)) {
    debugmsg("Couldn't create directory '%s'.", map_dir.c_str());
    return;
  }
  for (int sx = sector.x; sx < sector.x + 3; sx++) {
    for (int sy = sector.y; sy < sector.y + 3; sy++) {
      std::stringstream filename;
      filename << map_dir << "/map." << sx << "." << sy;
      std::ofstream fout;
      fout.open( filename.str().c_str() );
      if (!fout.is_open()) {
        debugmsg("Couldn't open '%s' for writing.", filename.str().c_str());
        return;
      }
      int start_x = sx * SECTOR_SIZE, start_y = sy * SECTOR_SIZE;
      for (int mx = start_x; mx < start_x + SECTOR_SIZE; mx++) {
        for (int my = start_y; my < start_y + SECTOR_SIZE; my++) {
          Tripoint curpos = Tripoint(mx, my, 0);
          while (point_map.count(curpos) > 0) {
            fout << curpos.x << " " << curpos.y << " " << curpos.z <<
                    std::endl << point_map[curpos]->save_data() << std::endl;
            curpos.z++;
          }
        }
      }
    }
  }
}

void Submap_pool::reset()
{
  for (std::list<Submap*>::iterator it = instances.begin();
       it != instances.end();
       it++) {
    delete (*it);
  }
  instances.clear();
  point_map.clear();
  sector = Point(-9999, -9999);
}

/**** Game ****/

std::string Game::save_file_for(std::string world_name)
{
  return SAVE_DIR + "/" + world_name + "/game.sav";
}

bool Game::has_save(std::string world_name)
{
  return file_exists( save_file_for(world_name) );
}

bool Game::save_game()
{
  if (!worldmap || !player || !map || game_over) {
    return false;
  }
  if (worldmap->get_name().empty()) {
    return false;
  }
// The world map holds the monster populations still to spawn.
  worldmap->save_to_name();
  SUBMAP_POOL.save_all();

  std::stringstream out;
  out << "Cataclysm2_save " << SAVE_FORMAT << std::endl;
  out << "Turn: " << time.get_turn() << std::endl;
  out << "Uids: " << next_item_uid << " " << next_furniture_uid << std::endl;
  out << "Map: " << map->posx << " " << map->posy << " " << map->posz <<
         std::endl;
  out << "Boat: " << boat.hull << " " << boat.sail << " " << boat.gps << " " <<
         boat.rations << " " << boat.drinks << std::endl;
  out << "Lab: " << lab.plant_on << " " << lab.substation_on << " " <<
         lab.open << std::endl;
  out << "Credit: " << trade_credit << std::endl;
  if (!camp_job.empty()) {
    out << "Job: " << camp_job << std::endl;
  }
  out << "Player:" << std::endl << player->save_data() << std::endl;
  for (std::list<Entity*>::iterator it = entities.instances.begin();
       it != entities.instances.end();
       it++) {
    Entity* ent = *it;
    if (ent->is_monster() && !ent->dead &&
        ent->pos.x >= 0 && ent->pos.y >= 0 &&
        ent->pos.x < SUBMAP_SIZE * MAP_SIZE &&
        ent->pos.y < SUBMAP_SIZE * MAP_SIZE) {
      out << "Monster:" << std::endl <<
             static_cast<Monster*>(ent)->save_data() << std::endl;
    }
  }
  int first = messages.size() > 20 ? messages.size() - 20 : 0;
  for (int i = first; i < messages.size(); i++) {
    out << "Message: " << messages[i].text << std::endl;
  }
  out << "Done" << std::endl;

// Write to a temporary file first, so a failed save never eats the old one.
  std::string filename = save_file_for(worldmap->get_name());
  std::string tmpname = filename + ".tmp";
  std::ofstream fout(tmpname.c_str());
  if (!fout.is_open()) {
    debugmsg("Couldn't open '%s' for saving.", tmpname.c_str());
    return false;
  }
  fout << out.str();
  fout.close();
  if (fout.fail() || std::rename(tmpname.c_str(), filename.c_str()) != 0) {
    debugmsg("Couldn't write the save file '%s'.", filename.c_str());
    return false;
  }
  return true;
}

void Game::delete_save()
{
  if (worldmap && !worldmap->get_name().empty()) {
    remove_file( save_file_for(worldmap->get_name()) );
  }
}

bool Game::load_game(std::string world_name)
{
  std::ifstream fin( save_file_for(world_name).c_str() );
  if (!fin.is_open()) {
    popup("There is no saved game in %s.", world_name.c_str());
    return false;
  }
  std::string header;
  int format = 0;
  fin >> header >> format;
  if (header != "Cataclysm2_save" || format < 1 || format > SAVE_FORMAT) {
    popup("The saved game in %s can't be read.", world_name.c_str());
    return false;
  }

  worldmap = new Worldmap;
  std::string world_file = SAVE_DIR + "/worlds/" + world_name + ".map";
  if (!worldmap->load_from_file(world_file)) {
    popup("Couldn't load the world map of %s.", world_name.c_str());
    return false;
  }

  map = new Map;
  player = new Player;   // not prep_new_character(): no starting clothes
  entities.clear();
  entities.add_entity(player);
  std::vector<Monster*> monsters;
  int mapx = 0, mapy = 0, mapz = 0, turn = 0;
  bool ok = true;

  std::string ident;
  while (ok && fin >> ident) {
    ident = no_caps(ident);
    if (ident == "done") {
      break;
    } else if (ident == "turn:") {
      fin >> turn;
    } else if (ident == "uids:") {
      fin >> next_item_uid >> next_furniture_uid;
    } else if (ident == "map:") {
      fin >> mapx >> mapy >> mapz;
    } else if (ident == "boat:") {
      int h, s, g;
      fin >> h >> s >> g >> boat.rations >> boat.drinks;
      boat.hull = (h != 0);
      boat.sail = (s != 0);
      boat.gps  = (g != 0);
    } else if (ident == "credit:") {
      fin >> trade_credit;
    } else if (ident == "job:") {
      camp_job = rest_of_line(fin);
    } else if (ident == "lab:") {
      int p, s, o;
      fin >> p >> s >> o;
      lab.plant_on = (p != 0);
      lab.substation_on = (s != 0);
      lab.open = (o != 0);
    } else if (ident == "player:") {
      ok = player->load_data(fin);
    } else if (ident == "monster:") {
      Monster* mon = new Monster;
      if (mon->load_data(fin)) {
        monsters.push_back(mon);
      } else {
        delete mon;
        ok = false;
      }
    } else if (ident == "message:") {
      messages.push_back( Game_message(rest_of_line(fin), 0) );
    } else {
      std::string junk;
      std::getline(fin, junk);
    }
  }
  if (!ok) {
    popup("The saved game in %s is damaged.", world_name.c_str());
    return false;
  }

  time = Time(turn);
  for (int i = 0; i < messages.size(); i++) {
    messages[i].turn = turn;
  }
  new_messages = 0;
// Load the submaps around the saved position, then the map itself; this
// also spawns whatever the world map still holds for these tiles.
  SUBMAP_POOL.load_area_centered_on(mapx + MAP_SIZE / 2, mapy + MAP_SIZE / 2);
  map->generate(worldmap, mapx, mapy, mapz);
  for (int i = 0; i < monsters.size(); i++) {
    entities.add_entity(monsters[i]);
  }
// Active items (a lit flashlight, ...) in the player's hands and pack.
  active_items.clear();
  if (player->weapon.is_active()) {
    add_active_item( &(player->weapon) );
  }
  for (int i = 0; i < player->inventory.size(); i++) {
    if (player->inventory[i].is_active()) {
      add_active_item( &(player->inventory[i]) );
    }
  }
  last_target = -1;
  game_over = false;
  add_msg("Welcome back, %s.", player->get_name().c_str());
  return true;
}

std::string Game::pick_saved_game()
{
  std::vector<std::string> saved;
  for (int i = 0; i < worldmap_names.size(); i++) {
    if (has_save(worldmap_names[i])) {
      saved.push_back(worldmap_names[i]);
    }
  }
  if (saved.empty()) {
    popup("There are no saved games yet.  Press N to start one.");
    return "";
  }
  if (saved.size() > 20) {
    saved.resize(20);
  }
  std::vector<std::string> options = saved;
  options.push_back("Cancel");
  int choice = menu_vec("Load which game?", options);
  if (choice < 0 || choice >= saved.size()) {
    return "";
  }
  return saved[choice];
}

bool Game::player_is_dead()
{
  return player && (player->current_hp[HP_PART_HEAD]  <= 0 ||
                    player->current_hp[HP_PART_TORSO] <= 0);
}

void Game::player_death()
{
  draw_all();
  int days = (time.get_turn() - Time(0, 0, 8, 1, SEASON_SPRING,
                                     STARTING_YEAR).get_turn()) / DAYS(1);
  std::string cause = player->last_damage.empty() ? "something" :
                                                    player->last_damage;
  popup("<c=ltred>You have died.<c=/>\n\
\n\
%s survived %s on the island.\n\
Killed by %s.\n\
\n\
Death is permanent: this game is over.", player->get_name().c_str(),
        days_text(days).c_str(), cause.c_str());
  delete_save();
  game_over = true;
  player->action_points = 0;
}
