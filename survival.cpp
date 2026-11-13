// survival.cpp: sleep, natural healing, starvation and dehydration, and
// stopping what you are doing when danger comes (Asciiverse, 2026).
//
// Whales' engine counted hunger, thirst and fatigue but had no way to sleep,
// no healing besides bandages, and nothing that killed you for going without
// food or water.  Whales wanted pressing clocks
// (https://whalesdev.tumblr.com/post/91112281541); these are they.

#include "game.h"
#include "player.h"
#include "globals.h"
#include "geometry.h"
#include "window.h"
#include "rng.h"

// Hunger and thirst past which the body starts to fail (hunger and thirst
// rise by one every six minutes): about six days without food, two without
// water.
#define STARVING_HUNGER   1440
#define DEHYDRATED_THIRST  480

bool Player::is_sleeping()
{
  return activity.is_active() && activity.type == PLAYER_ACTIVITY_SLEEP;
}

void Player::body_upkeep()
{
  bool sleeping = is_sleeping();
// Sleeping pays fatigue back: start_turn() added one point every six
// minutes; asleep we take three off instead, a good night's sleep for a
// long day.
  if (sleeping && GAME.minute_timer(6)) {
    fatigue -= 4;   // the +1 from start_turn() makes it -3
    if (fatigue < 0) {
      fatigue = 0;
    }
  }

// Starvation and thirst wear the body down.
  if (GAME.minute_timer(10)) {
    if (hunger >= STARVING_HUNGER) {
      take_damage_no_armor(DAMAGE_BASH, 1, "starvation", BODY_PART_TORSO);
      if (!sleeping) {
        GAME.add_msg("<c=red>You are starving to death!<c=/>");
      }
    }
    if (thirst >= DEHYDRATED_THIRST) {
      take_damage_no_armor(DAMAGE_BASH, 1, "thirst", BODY_PART_TORSO);
      if (!sleeping) {
        GAME.add_msg("<c=red>You are dying of thirst!<c=/>");
      }
    }
  }

// Natural healing: slow awake, faster asleep, none when starving or
// parched.
  int interval = sleeping ? 30 : 120;
  if (GAME.minute_timer(interval) && hunger < 480 && thirst < 240) {
    for (int i = HP_PART_HEAD; i < HP_PART_MAX; i++) {
      if (current_hp[i] > 0 && current_hp[i] < max_hp[i]) {
        current_hp[i]++;
      }
    }
  }
}

// A monster close enough to be a danger, that we can see (or, asleep, hear).
bool Game::danger_near()
{
  bool sleeping = player->is_sleeping();
  for (std::list<Entity*>::iterator it = entities.instances.begin();
       it != entities.instances.end();
       it++) {
    Entity* ent = *it;
    if (ent == player || ent->dead || !ent->is_monster()) {
      continue;
    }
    int dist = rl_dist(player->pos, ent->pos);
    if (sleeping) {
      if (dist <= 4) {
        return true;
      }
    } else if (dist <= 15 && player->can_see(map, ent->pos)) {
      return true;
    }
  }
  return false;
}

void Game::interrupt_activity(std::string why)
{
  if (!player->has_activity()) {
    return;
  }
  if (player->is_sleeping()) {
    add_msg("<c=yellow>You wake up%s<c=/>", why.c_str());
  } else {
    add_msg("<c=yellow>You stop %s%s<c=/>",
            player->activity.get_participle().c_str(), why.c_str());
  }
  player->activity = Player_activity();
}

void Game::start_sleeping()
{
  if (player->fatigue < 40) {
    add_msg("You're not tired enough to sleep.");
    return;
  }
  if (danger_near()) {
    add_msg("You can't sleep with danger this close!");
    return;
  }
  add_msg("You lie down and go to sleep.");
// The activity ends when we are rested (handle_player_activity()), or
// after twelve hours at most.
  player->set_activity(PLAYER_ACTIVITY_SLEEP, HOURS(12) * 100);
}
