#include "mapgen.hpp"

#include "init.hpp"

#include <algorithm>
#include <climits>

#include "room.hpp"
#include "mapgen.hpp"
#include "feature_event.hpp"
#include "actor_player.hpp"
#include "feature_door.hpp"
#include "actor_factory.hpp"
#include "actor_mon.hpp"
#include "drop.hpp"
#include "item_factory.hpp"
#include "map.hpp"
#include "map_parsing.hpp"
#include "io.hpp"
#include "populate_monsters.hpp"
#include "populate_traps.hpp"
#include "populate_items.hpp"
#include "gods.hpp"
#include "rl_utils.hpp"

#ifdef DEMO_MODE
#include "io.hpp"
#include "sdl_base.hpp"
#include "query.hpp"
#endif // DEMO_MODE

namespace mapgen
{

bool door_proposals[map_w][map_h];

namespace
{

void connect_rooms()
{
    TRACE_FUNC_BEGIN;

    int nr_tries_left = 5000;

    while (true)
    {
        // NOTE: Keep this counter at the top of the loop, since otherwise a
        //       continue statement could bypass it so we get stuck in the loop.
        --nr_tries_left;

        if (nr_tries_left == 0)
        {
            mapgen::is_map_valid = false;

#ifdef DEMO_MODE
            io::cover_panel(Panel::log);
            states::draw();
            io::draw_text("Failed to connect map",
                          Panel::screen,
                          P(0, 0),
                          clr_red_lgt);
            io::update_screen();
            sdl_base::sleep(8000);
#endif // DEMO_MODE
            break;
        }

        auto rnd_room = []()
        {
            return map::room_list[rnd::range(0, map::room_list.size() - 1)];
        };

        // Standard rooms are connectable
        auto is_connectable_room = [](const Room & r)
        {
            return (int)r.type_ < (int)RoomType::END_OF_STD_ROOMS;
        };

        Room* room0 = rnd_room();

        // Room 0 must be a connectable room, or a corridor link
        if (!is_connectable_room(*room0) &&
            room0->type_ != RoomType::corr_link)
        {
            continue;
        }

        // Finding second room to connect to
        Room* room1 = rnd_room();

        // Room 1 must not be the same as room 0, and it must be a connectable
        // room (connections are only allowed between two standard rooms, or
        // from a corridor link to a standard room - never between two corridor
        // links)
        while ((room1 == room0) ||
               !is_connectable_room(*room1))
        {
            room1 = rnd_room();
        }

        // Do not allow two rooms to be connected twice
        const auto& room0_connections = room0->rooms_con_to_;

        if (find(room0_connections.begin(),
                 room0_connections.end(),
                 room1) != room0_connections.end())
        {
            // Rooms are already connected, trying other combination
            continue;
        }

        // Do not connect room 0 and 1 if another room (except for sub rooms)
        // lies anywhere in a rectangle defined by the two center points of
        // those rooms.
        bool is_other_room_in_way = false;

        const P c0(room0->r_.center());
        const P c1(room1->r_.center());

        const int x0 = std::min(c0.x, c1.x);
        const int y0 = std::min(c0.y, c1.y);
        const int x1 = std::max(c0.x, c1.x);
        const int y1 = std::max(c0.y, c1.y);

        for (int x = x0; x <= x1; ++x)
        {
            for (int y = y0; y <= y1; ++y)
            {
                const Room* const room_here = map::room_map[x][y];

                if (room_here &&
                    room_here != room0 &&
                    room_here != room1 &&
                    !room_here->is_sub_room_)
                {
                    is_other_room_in_way = true;
                    break;
                }
            }

            if (is_other_room_in_way)
            {
                break;
            }
        }

        if (is_other_room_in_way)
        {
            // Blocked by room between, trying other combination
            continue;
        }

        // Alright, let's try to connect these rooms
        mk_pathfind_corridor(*room0,
                             *room1,
                             door_proposals);

        bool blocked[map_w][map_h];

        map_parsers::BlocksMoveCommon(ParseActors::no).
            run(blocked);

        //
        // Do not consider doors blocking
        //
        for (int x = 0; x < map_w; ++x)
        {
            for (int y = 0; y < map_h; ++y)
            {
                if (map::cells[x][y].rigid->id() == FeatureId::door)
                {
                    blocked[x][y] = false;
                }
            }
        }

        if ((nr_tries_left <= 2 || rnd::one_in(4)) &&
            map_parsers::is_map_connected(blocked))
        {
            break;
        }
    }

    TRACE_FUNC_END;
}

void allowed_stair_cells(bool out[map_w][map_h])
{
    TRACE_FUNC_BEGIN;

    // Mark cells as free if all adjacent feature types are allowed
    std::vector<FeatureId> feat_ids_ok
    {
        FeatureId::floor,
        FeatureId::carpet,
        FeatureId::grass
    };

    map_parsers::AllAdjIsAnyOfFeatures(feat_ids_ok)
        .run(out);

    // Block cells with item
    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            if (map::cells[x][y].item)
            {
                out[x][y] = false;
            }
        }
    }

    // Block cells with actors
    for (const auto* const actor : game_time::actors)
    {
        const P& p(actor->pos);
        out[p.x][p.y] = false;
    }

    TRACE_FUNC_END;
}

P mk_stairs()
{
    TRACE_FUNC_BEGIN;

    bool allowed_cells[map_w][map_h];

    allowed_stair_cells(allowed_cells);

    auto allowed_cells_list = to_vec(allowed_cells, true);

    const int nr_ok_cells = allowed_cells_list.size();

    const int min_nr_ok_cells_req = 3;

    if (nr_ok_cells < min_nr_ok_cells_req)
    {
        TRACE << "Nr available cells to place stairs too low "
              << "(" << nr_ok_cells << "), discarding map" << std:: endl;
        is_map_valid = false;
#ifdef DEMO_MODE
        io::cover_panel(Panel::log);
        states::draw();
        io::draw_text("To few cells to place stairs",
                          Panel::screen,
                          P(0, 0),
                          clr_red_lgt);
        io::update_screen();
        sdl_base::sleep(8000);
#endif // DEMO_MODE
        return P(-1, -1);
    }

    TRACE << "Sorting the allowed cells vector "
          << "(" << allowed_cells_list.size() << " cells)" << std:: endl;

    IsCloserToPos is_closer(map::player->pos);

    std::sort(allowed_cells_list.begin(),
              allowed_cells_list.end(),
              is_closer);

    TRACE << "Picking furthest cell" << std:: endl;
    const P stairs_pos(allowed_cells_list[nr_ok_cells - 1]);

    TRACE << "Spawning stairs at chosen cell" << std:: endl;
    map::put(new Stairs(stairs_pos));

    TRACE_FUNC_END;
    return stairs_pos;
}

void move_player_to_nearest_allowed_pos()
{
    TRACE_FUNC_BEGIN;

    bool allowed_cells[map_w][map_h];
    allowed_stair_cells(allowed_cells);

    auto allowed_cells_list = to_vec(allowed_cells, true);

    if (allowed_cells_list.empty())
    {
        is_map_valid = false;
    }
    else // Valid cells exists
    {
        TRACE << "Sorting the allowed cells vector "
              << "(" << allowed_cells_list.size() << " cells)" << std:: endl;

        IsCloserToPos is_closer_to_origin(map::player->pos);

        sort(allowed_cells_list.begin(),
             allowed_cells_list.end(),
             is_closer_to_origin);

        map::player->pos = allowed_cells_list.front();

    }

    TRACE_FUNC_END;
}

void reveal_doors_on_path_to_stairs(const P& stairs_pos)
{
    TRACE_FUNC_BEGIN;

    bool blocked[map_w][map_h];

    map_parsers::BlocksMoveCommon(ParseActors::no)
        .run(blocked);

    blocked[stairs_pos.x][stairs_pos.y] = false;

    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            if (map::cells[x][y].rigid->id() == FeatureId::door)
            {
                blocked[x][y] = false;
            }
        }
    }

    std::vector<P> path;

    pathfind(map::player->pos,
             stairs_pos,
             blocked,
             path);

    ASSERT(!path.empty());

    TRACE << "Travelling along path and revealing all doors" << std:: endl;

    for (P& pos : path)
    {
        auto* const feature = map::cells[pos.x][pos.y].rigid;

        if (feature->id() == FeatureId::door)
        {
            static_cast<Door*>(feature)->reveal(Verbosity::silent);
        }
    }

    TRACE_FUNC_END;
}

} // namespace

bool mk_std_lvl()
{
    TRACE_FUNC_BEGIN;

    is_map_valid = true;

    io::clear_screen();
    io::update_screen();

    map::reset_map();

    TRACE << "Resetting helper arrays" << std:: endl;

    std::fill_n(*door_proposals, nr_map_cells, false);

    //
    // NOTE: This must be called before any rooms are created
    //
    room_factory::init_room_bucket();

    TRACE << "Init regions" << std:: endl;
    const int map_w_third = map_w / 3;
    const int map_h_third = map_h / 3;

    const int spl_x0 = map_w_third;
    const int spl_x1 = (map_w_third * 2) + 1;
    const int spl_y0 = map_h_third;
    const int spl_y1 = map_h_third * 2;

    Region regions[3][3];

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            const R r((x == 0) ? 1 :
                      (x == 1) ? (spl_x0 + 1) : (spl_x1 + 1),

                      (y == 0) ? 1 :
                      (y == 1) ? (spl_y0 + 1) : spl_y1 + 1,

                      (x == 0) ? (spl_x0 - 1) :
                      (x == 1) ? (spl_x1 - 1) : map_w - 2,

                      (y == 0) ? (spl_y0 - 1) :
                      (y == 1) ? (spl_y1 - 1) : (map_h - 2));

            regions[x][y] = Region(r);
        }
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Reserve regions for a "river"
    // -------------------------------------------------------------------------
#ifndef DISABLE_MK_RIVER
    const int river_one_in_n = 8;

    if (map::dlvl >= dlvl_first_mid_game &&
        rnd::one_in(river_one_in_n))
    {
        reserve_river(regions);
    }

    if (!is_map_valid)
    {
        return false;
    }
#endif // MK_RIVER

    // -------------------------------------------------------------------------
    // Merge some regions
    // -------------------------------------------------------------------------
#ifndef DISABLE_MERGED_REGIONS
    merge_regions(regions);

    if (!is_map_valid)
    {
        return false;
    }
#endif // MK_MERGED_REGIONS

    // -------------------------------------------------------------------------
    // Make main rooms
    // -------------------------------------------------------------------------
    TRACE << "Making main rooms" << std:: endl;

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            auto& region = regions[x][y];

            if (!region.main_room && region.is_free)
            {
                mk_room(region);
            }
        }
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Make auxiliary rooms
    // -------------------------------------------------------------------------
#ifndef DISABLE_AUX_ROOMS
#ifdef DEMO_MODE
    io::cover_panel(Panel::log);
    states::draw();
    io::draw_text("Press any key to make aux rooms...",
                      Panel::screen,
                      P(0, 0),
                      clr_white);
    io::update_screen();
    query::wait_for_key_press();
#endif // DEMO_MODE

    mk_aux_rooms(regions);

    // -------------------------------------------------------------------------
    // Make sub-rooms
    // -------------------------------------------------------------------------
    if (!is_map_valid)
    {
        return false;
    }
#endif // DISABLE_AUX_ROOMS

#ifndef DISABLE_MK_SUB_ROOMS
    if (map::dlvl <= dlvl_last_mid_game)
    {
#ifdef DEMO_MODE
        io::cover_panel(Panel::log);
        states::draw();
        io::draw_text("Press any key to make sub rooms...",
                          Panel::screen,
                          P(0, 0),
                          clr_white);
        io::update_screen();
        query::wait_for_key_press();
#endif // DEMO_MODE

        mk_sub_rooms();
    }

    if (!is_map_valid)
    {
        return false;
    }
#endif // DISABLE_MK_SUB_ROOMS

    TRACE << "Sorting the room list according to room type" << std:: endl;
    // NOTE: This allows common rooms to assume that they are rectangular and
    //       have their walls untouched when their reshaping functions run.
    auto cmp = [](const Room * r0, const Room * r1)
    {
        return (int)r0->type_ < (int)r1->type_;
    };

    sort(map::room_list.begin(), map::room_list.end(), cmp);

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Run the pre-connect hook on all rooms
    // -------------------------------------------------------------------------
    TRACE << "Running pre-connect functions for all rooms" << std:: endl;

#ifdef DEMO_MODE
    io::cover_panel(Panel::log);
    states::draw();
    io::draw_text("Press any key to run pre-connect functions on rooms...",
                      Panel::screen,
                      P(0, 0),
                      clr_white);
    io::update_screen();
    query::wait_for_key_press();
#endif // DEMO_MODE

    gods::set_no_god();

    for (Room* room : map::room_list)
    {
        room->on_pre_connect(door_proposals);
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Connect the rooms
    // -------------------------------------------------------------------------
#ifdef DEMO_MODE
    io::cover_panel(Panel::log);
    states::draw();
    io::draw_text("Press any key to connect rooms...",
                  Panel::screen,
                  P(0, 0),
                  clr_white);
    io::update_screen();
    query::wait_for_key_press();
#endif // DEMO_MODE

    connect_rooms();

    // -------------------------------------------------------------------------
    // Run the post-connect hook on all rooms
    // -------------------------------------------------------------------------
    if (!is_map_valid)
    {
        return false;
    }

    TRACE << "Running post-connect functions for all rooms" << std:: endl;
#ifdef DEMO_MODE
    io::cover_panel(Panel::log);
    states::draw();
    io::draw_text("Press any key to run post-connect functions on rooms...",
                  Panel::screen,
                  P(0, 0),
                  clr_white);
    io::update_screen();
    query::wait_for_key_press();
#endif // DEMO_MODE

    for (Room* room : map::room_list)
    {
        room->on_post_connect(door_proposals);
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Place doors
    // -------------------------------------------------------------------------
    if (map::dlvl <= dlvl_last_mid_game)
    {
        mk_doors();
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Move player to the nearest free position
    // -------------------------------------------------------------------------
    move_player_to_nearest_allowed_pos();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Decorate the map
    // -------------------------------------------------------------------------
#ifndef DISABLE_DECORATE
    decorate();

    if (!is_map_valid)
    {
        return false;
    }
#endif // DISABLE_DECORATE

    // -------------------------------------------------------------------------
    // Place the stairs
    // -------------------------------------------------------------------------
    // NOTE: The choke point information gathering below depends on the stairs
    //       having been placed.
    //
    P stairs_pos;

    stairs_pos = mk_stairs();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Gather data on choke points in the map (check every position where a door
    // has previously been "proposed")
    // -------------------------------------------------------------------------
    bool blocked[map_w][map_h];

    map_parsers::BlocksMoveCommon(ParseActors::no)
        .run(blocked);

    // Consider stairs and doors as non-blocking
    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            const FeatureId id = map::cells[x][y].rigid->id();

            if (id == FeatureId::stairs ||
                id == FeatureId::door)
            {
                blocked[x][y] = false;
            }
        }
    }

    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            if (!blocked[x][y] && door_proposals[x][y])
            {
                ChokePointData d;

                const bool is_choke =
                    is_choke_point(P(x, y),
                                   blocked,
                                   d);

                // 'is_choke_point' called above may invalidate the map
                if (!is_map_valid)
                {
                    return false;
                }

                if (is_choke)
                {
                    // Find player and stair side
                    for (size_t side_idx = 0; side_idx < 2; ++side_idx)
                    {
                        for (const P& p : d.sides[side_idx])
                        {
                            if (p == map::player->pos)
                            {
                                ASSERT(d.player_side == -1);

                                d.player_side = side_idx;
                            }

                            if (p == stairs_pos)
                            {
                                ASSERT(d.stairs_side == -1);

                                d.stairs_side = side_idx;
                            }
                        }
                    }

                    ASSERT(d.player_side == 0 || d.player_side == 1);
                    ASSERT(d.stairs_side == 0 || d.stairs_side == 1);

                    // Robustness for release mode
                    if ((d.player_side != 0 && d.player_side != 1) ||
                        (d.player_side != 0 && d.player_side != 1))
                    {
                        // Go to next map position
                        continue;
                    }

                    map::choke_point_data.emplace_back(d);
                }
            }
        } // y loop
    } // x loop

    TRACE << "Found " << map::choke_point_data.size()
          << " choke points" << std::endl;

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Make metal doors and levers
    // -------------------------------------------------------------------------
    mk_metal_doors_and_levers();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Explicitly make some doors leading to "optional" areas secret or stuck
    // -------------------------------------------------------------------------
    for (const auto& choke_point : map::choke_point_data)
    {
        if (choke_point.player_side == choke_point.stairs_side)
        {
            Rigid* const rigid =
                map::cells[choke_point.p.x][choke_point.p.y].rigid;

            if (rigid->id() == FeatureId::door)
            {
                Door* const door = static_cast<Door*>(rigid);

                if ((door->type() != DoorType::gate) &&
                    (door->type() != DoorType::metal) &&
                    rnd::one_in(6))
                {
                    door->set_secret();
                }

                if (rnd::one_in(6))
                {
                    door->set_stuck();
                }
            }
        }
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Place Monoliths
    // -------------------------------------------------------------------------
    // NOTE: This depends on choke point data having been gathered (including
    //       player side and stairs side)
    //
    mk_monoliths();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Place pylons and levers
    // -------------------------------------------------------------------------
    mk_pylons_and_levers();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Populate the map with monsters
    // -------------------------------------------------------------------------
    populate_mon::populate_std_lvl();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Populate the map with traps
    // -------------------------------------------------------------------------
    populate_traps::populate_std_lvl();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Populate the map with items on the floor
    // -------------------------------------------------------------------------
    populate_items::mk_items_on_floor();

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Place "snake emerge" events
    // -------------------------------------------------------------------------
    const int nr_snake_emerge_events_to_try =
        rnd::one_in(30) ? 2 :
        rnd::one_in(8)  ? 1 : 0;

    for (int i = 0; i < nr_snake_emerge_events_to_try; ++i)
    {
        EventSnakeEmerge* const event = new EventSnakeEmerge();

        if (event->try_find_p())
        {
            game_time::add_mob(event);
        }
        else
        {
            delete event;
        }
    }

    if (!is_map_valid)
    {
        return false;
    }

    // -------------------------------------------------------------------------
    // Reveal all doors on the path to the stairs (if "early" dungeon level)
    // -------------------------------------------------------------------------
    const int last_lvl_to_reveal_stairs_path = 6;

    if (map::dlvl <= last_lvl_to_reveal_stairs_path)
    {
        reveal_doors_on_path_to_stairs(stairs_pos);
    }

    // -------------------------------------------------------------------------
    // Occasionally make the whole level dark
    // -------------------------------------------------------------------------
    if (map::dlvl > 1)
    {
        const int make_drk_pct = 2 + (map::dlvl / 4);

        if (rnd::percent(make_drk_pct))
        {
            for (int x = 0; x < map_w; ++x)
            {
                for (int y = 0; y < map_h; ++y)
                {
                    map::cells[x][y].is_dark = true;
                }
            }
        }
    }

    if (!is_map_valid)
    {
        return false;
    }

    for (auto* r : map::room_list)
    {
        delete r;
    }

    map::room_list.clear();

    std::fill_n(*map::room_map, nr_map_cells, nullptr);

    TRACE_FUNC_END;
    return is_map_valid;
}

} // mapgen
