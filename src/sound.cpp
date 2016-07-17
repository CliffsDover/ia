#include "sound.hpp"

#include <iostream>
#include <string>

#include "feature_rigid.hpp"
#include "map.hpp"
#include "actor_player.hpp"
#include "actor_mon.hpp"
#include "game_time.hpp"
#include "map_parsing.hpp"

Snd::Snd(
    const std::string&              msg,
    const Sfx_id                    sfx,
    const Ignore_msg_if_origin_seen ignore_msg_if_origin_seen,
    const P&                      origin,
    Actor* const                    actor_who_made_sound,
    const Snd_vol                   vol,
    const Alerts_mon                alerting_mon,
    const More_prompt_on_msg        add_more_prompt_on_msg) :

    msg_                            (msg),
    sfx_                            (sfx),
    is_msg_ignored_if_origin_seen_  (ignore_msg_if_origin_seen),
    origin_                         (origin),
    actor_who_made_sound_           (actor_who_made_sound),
    vol_                            (vol),
    is_alerting_mon_                (alerting_mon),
    add_more_prompt_on_msg_         (add_more_prompt_on_msg) {}

namespace snd_emit
{

namespace
{

int nr_snd_msg_printed_cur_turn_;

bool is_snd_heard_at_range(const int RANGE, const Snd& snd)
{
    return RANGE <= (snd.is_loud() ? snd_dist_loud : snd_dist_normal);
}

} //namespace

void reset_nr_snd_msg_printed_cur_turn()
{
    nr_snd_msg_printed_cur_turn_ = 0;
}

void run(Snd snd)
{
    bool blocked[map_w][map_h];

    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            const auto f  = map::cells[x][y].rigid;
            blocked[x][y] = !f->is_sound_passable();
        }
    }

    int floodfill[map_w][map_h];

    const P& origin = snd.origin();

    floodfill::run(origin, blocked, floodfill, 999, P(-1, -1), true);

    floodfill[origin.x][origin.y] = 0;

    for (Actor* actor : game_time::actors)
    {
        const int FLOOD_VAL_AT_ACTOR = floodfill[actor->pos.x][actor->pos.y];

        const bool IS_ORIGIN_SEEN_BY_PLAYER =
            map::cells[origin.x][origin.y].is_seen_by_player;

        if (is_snd_heard_at_range(FLOOD_VAL_AT_ACTOR, snd))
        {
            if (actor->is_player())
            {
                if ((IS_ORIGIN_SEEN_BY_PLAYER && snd.is_msg_ignored_if_origin_seen()))
                {
                    snd.clear_msg();
                }

                const P& player_pos = map::player->pos;

                if (!snd.msg().empty())
                {
                    //Add a direction string to the message (i.e. "(NW)", "(E)" , etc)
                    if (player_pos != origin)
                    {
                        std::string dir_str = "";
                        dir_utils::compass_dir_name(player_pos, origin, dir_str);
                        snd.add_string("(" + dir_str + ")");
                    }
                }

                const int SND_MAX_DIST  = snd.is_loud() ? snd_dist_loud : snd_dist_normal;

                const int PCT_DIST      = (FLOOD_VAL_AT_ACTOR * 100) / SND_MAX_DIST;

                const P offset = (origin - player_pos).signs();

                const Dir dir_to_origin = dir_utils::dir(offset);

                map::player->hear_sound(snd, IS_ORIGIN_SEEN_BY_PLAYER, dir_to_origin, PCT_DIST);
            }
            else //Not player
            {
                Mon* const mon = static_cast<Mon*>(actor);
                mon->hear_sound(snd);
            }
        }
    }
}

}
