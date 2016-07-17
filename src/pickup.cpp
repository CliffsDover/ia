#include "pickup.hpp"

#include <string>

#include "init.hpp"
#include "item.hpp"
#include "map.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "query.hpp"
#include "drop.hpp"
#include "item_factory.hpp"
#include "game_time.hpp"
#include "audio.hpp"

namespace item_pickup
{

void try_pick()
{
    const P& pos = map::player->pos;
    Item* const item = map::cells[pos.x][pos.y].item;

    if (item)
    {
        Inventory& player_inv = map::player->inv();

        const std::string item_name = item->name(Item_ref_type::plural);

        //First try to add it to carried item stack in thrown slot.
        Item* const carried_missile = player_inv.item_in_slot(Slot_id::thrown);

        if (
            item->data().is_stackable   &&
            carried_missile             &&
            item->id() == carried_missile->data().id)
        {
            audio::play(Sfx_id::pickup);

            msg_log::add("I add " + item_name + " to my missile stack.");
            carried_missile->nr_items_ += item->nr_items_;
            delete item;
            map::cells[pos.x][pos.y].item = nullptr;
            game_time::tick();
            return;
        }

        audio::play(Sfx_id::pickup);

        msg_log::clear();
        msg_log::add("I pick up " + item_name + ".");

        //Calls the itemps ickup hook
        //NOTE: This may destroy the item (e.g. combine with others)
        player_inv.put_in_backpack(item);

        map::cells[pos.x][pos.y].item = nullptr;

        game_time::tick();
    }
    else //No item in this cell
    {
        msg_log::clear();
        msg_log::add("I see nothing to pick up here.");
    }
}

Ammo* unload_ranged_wpn(Wpn& wpn)
{
    ASSERT(!wpn.data().ranged.has_infinite_ammo);

    const int nr_ammo_loaded = wpn.nr_ammo_loaded_;

    if (nr_ammo_loaded == 0)
    {
        return nullptr;
    }

    const Item_id   ammo_id         = wpn.data().ranged.ammo_item_id;
    Item_data_t&    ammo_data       = item_data::data[int(ammo_id)];
    Item*           spawned_ammo    = item_factory::mk(ammo_id);

    if (ammo_data.type == Item_type::ammo_mag)
    {
        //Unload a mag
        static_cast<Ammo_mag*>(spawned_ammo)->ammo_ = nr_ammo_loaded;
    }
    else
    {
        //Unload loose ammo
        spawned_ammo->nr_items_ = nr_ammo_loaded;
    }

    wpn.nr_ammo_loaded_ = 0;

    return static_cast<Ammo*>(spawned_ammo);
}

void try_unload_wpn_or_pickup_ammo()
{
    Item* item = map::cells[map::player->pos.x][map::player->pos.y].item;

    if (item)
    {
        if (item->data().ranged.is_ranged_wpn)
        {
            Wpn* const wpn = static_cast<Wpn*>(item);

            const std::string wpn_name = wpn->name(Item_ref_type::a, Item_ref_inf::yes);

            if (!wpn->data().ranged.has_infinite_ammo)
            {
                Ammo* const spawned_ammo = unload_ranged_wpn(*wpn);

                if (spawned_ammo)
                {
                    audio::play(Sfx_id::pickup);

                    msg_log::add("I unload " + wpn_name + ".");

                    //Calls items pickup hook
                    //NOTE: This may destroy the item (e.g. combine with others)
                    map::player->inv().put_in_backpack(spawned_ammo);

                    game_time::tick();
                    return;
                }
            }
        }
        else //Not a ranged weapon
        {
            if (
                item->data().type == Item_type::ammo ||
                item->data().type == Item_type::ammo_mag)
            {
                try_pick();
                return;
            }
        }
    }

    msg_log::add("I see no ammo to unload or pick up here.");
}

} //Item_pickup
