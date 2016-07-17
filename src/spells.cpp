#include "spells.hpp"

#include <algorithm>
#include <vector>

#include "init.hpp"
#include "render.hpp"
#include "actor_mon.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "postmortem.hpp"
#include "knockback.hpp"
#include "map.hpp"
#include "actor_factory.hpp"
#include "feature_trap.hpp"
#include "player_spells_handling.hpp"
#include "item_scroll.hpp"
#include "inventory.hpp"
#include "map_parsing.hpp"
#include "line_calc.hpp"
#include "sdl_wrapper.hpp"
#include "player_bon.hpp"
#include "dungeon_master.hpp"

namespace
{

const int SUMMON_HOSTILE_ONE_IN_N = 7;

} //namespace

namespace spell_handling
{

Spell* random_spell_for_mon()
{
    std::vector<Spell_id> bucket;

    for (int i = 0; i < int(Spell_id::END); ++i)
    {
        Spell* const spell = mk_spell_from_id(Spell_id(i));

        if (spell->is_avail_for_all_mon())
        {
            bucket.push_back(Spell_id(i));
        }

        delete spell;
    }

    ASSERT(!bucket.empty());

    const int ELEMENT = rnd::range(0, bucket.size() - 1);

    return mk_spell_from_id(bucket[ELEMENT]);
}

Spell* mk_spell_from_id(const Spell_id spell_id)
{
    switch (spell_id)
    {
    case Spell_id::slow_mon:
        return new Spell_slow_mon;

    case Spell_id::terrify_mon:
        return new Spell_terrify_mon;

    case Spell_id::paralyze_mon:
        return new Spell_paralyze_mon;

    case Spell_id::disease:
        return new Spell_disease;

    case Spell_id::darkbolt:
        return new Spell_darkbolt;

    case Spell_id::aza_wrath:
        return new Spell_aza_wrath;

    case Spell_id::summon:
        return new Spell_summon_mon;

    case Spell_id::heal_self:
        return new Spell_heal_self;

    case Spell_id::knock_back:
        return new Spell_knock_back;

    case Spell_id::teleport:
        return new Spell_teleport;

    case Spell_id::mayhem:
        return new Spell_mayhem;

    case Spell_id::pest:
        return new Spell_pest;

    case Spell_id::anim_wpns:
        return new Spell_anim_wpns;

    case Spell_id::det_items:
        return new Spell_det_items;

    case Spell_id::det_traps:
        return new Spell_det_traps;

    case Spell_id::det_mon:
        return new Spell_det_mon;

    case Spell_id::opening:
        return new Spell_opening;

    case Spell_id::sacr_life:
        return new Spell_sacr_life;

    case Spell_id::sacr_spi:
        return new Spell_sacr_spi;

    case Spell_id::cloud_minds:
        return new Spell_cloud_minds;

    case Spell_id::frenzy:
        return new Spell_frenzy;

    case Spell_id::bless:
        return new Spell_bless;

    case Spell_id::mi_go_hypno:
        return new Spell_mi_go_hypno;

    case Spell_id::burn:
        return new Spell_burn;

    case Spell_id::res:
        return new Spell_res;

    case Spell_id::pharaoh_staff:
        return new Spell_pharaoh_staff;

    case Spell_id::light:
        return new Spell_light;

    case Spell_id::END:
        break;
    }

    ASSERT(false && "No spell found for ID");
    return nullptr;
}

} //Spell_handling

Range Spell::spi_cost(const bool IS_BASE_COST_ONLY, Actor* const caster) const
{
    int cost_max = max_spi_cost();

    if (caster == map::player && !IS_BASE_COST_ONLY)
    {
        const int X0 = std::max(0, caster->pos.x - 1);
        const int Y0 = std::max(0, caster->pos.y - 1);
        const int X1 = std::min(map_w - 1, caster->pos.x + 1);
        const int Y1 = std::min(map_h - 1, caster->pos.y + 1);

        for (int x = X0; x <= X1; ++x)
        {
            for (int y = Y0; y <= Y1; ++y)
            {
                if (map::cells[x][y].rigid->id() == Feature_id::altar)
                {
                    cost_max -= 1;
                }
            }
        }

        bool IS_WARLOCK     = player_bon::traits[size_t(Trait::warlock)];
        bool IS_BLOOD_SORC  = player_bon::traits[size_t(Trait::blood_sorcerer)];
        bool IS_SEER        = player_bon::traits[size_t(Trait::seer)];
        bool IS_SUMMONER    = player_bon::traits[size_t(Trait::summoner)];

        if (IS_BLOOD_SORC)
        {
            --cost_max;
        }

        switch (id())
        {
        case Spell_id::darkbolt:
            if (IS_WARLOCK)
            {
                --cost_max;
            }
            break;

        case Spell_id::aza_wrath:
            if (IS_WARLOCK)
            {
                --cost_max;
            }
            break;

        case Spell_id::mayhem:
            if (IS_WARLOCK)
            {
                --cost_max;
            }
            break;

        case Spell_id::det_mon:
            if (IS_SEER)
            {
                --cost_max;
            }
            break;

        case Spell_id::det_items:
            if (IS_SEER)
            {
                cost_max -= 3;
            }
            break;

        case Spell_id::det_traps:
            if (IS_SEER)
            {
                cost_max -= 3;
            }
            break;

        case Spell_id::summon:
            if (IS_SUMMONER)
            {
                --cost_max;
            }
            break;

        case Spell_id::pest:
            if (IS_SUMMONER)
            {
                --cost_max;
            }
            break;

        case Spell_id::pharaoh_staff:
            if (IS_SUMMONER)
            {
                --cost_max;
            }
            break;

        default:
            break;
        }

        Prop_handler& prop_hlr = caster->prop_handler();

        if (!prop_hlr.allow_see())
        {
            --cost_max;
        }

        if (caster->has_prop(Prop_id::blessed))
        {
            --cost_max;
        }

        if (caster->has_prop(Prop_id::cursed))
        {
            cost_max += 3;
        }
    }

    cost_max            = std::max(1, cost_max);
    const int COST_MIN  = std::max(1, (cost_max + 1) / 2);

    return Range(COST_MIN, cost_max);
}

Spell_effect_noticed Spell::cast(Actor* const caster, const bool IS_INTRINSIC) const
{
    TRACE_FUNC_BEGIN;

    ASSERT(caster);

    if (caster->prop_handler().allow_cast_spell(Verbosity::verbose))
    {
        if (caster->is_player())
        {
            TRACE << "Player casting spell" << std::endl;

            const Shock_src shock_src = IS_INTRINSIC ?
                                        Shock_src::cast_intr_spell :
                                        Shock_src::use_strange_item;

            const int SHOCK_VALUE = IS_INTRINSIC ? shock_lvl_intr_cast() : 10;

            map::player->incr_shock(SHOCK_VALUE, shock_src);

            Snd snd("",
                    Sfx_id::spell_generic,
                    Ignore_msg_if_origin_seen::yes,
                    caster->pos,
                    caster,
                    Snd_vol::low,
                    Alerts_mon::yes);

            snd_emit::run(snd);
        }
        else //Caster is monster
        {
            TRACE << "Monster casting spell" << std::endl;
            Mon* const mon = static_cast<Mon*>(caster);

            const bool IS_MON_SEEN = map::player->can_see_actor(*mon);

            std::string spell_str = mon->data().spell_cast_msg;

            if (!spell_str.empty())
            {
                std::string mon_name = "";

                if (IS_MON_SEEN)
                {
                    mon_name = mon->name_the();
                }
                else //Cannot see monster
                {
                    mon_name = mon->data().is_humanoid ? "Someone" : "Something";
                }

                spell_str = mon_name + " " + spell_str;
            }

            Snd snd(spell_str,
                    Sfx_id::END,
                    Ignore_msg_if_origin_seen::no,
                    caster->pos,
                    caster,
                    Snd_vol::low,
                    Alerts_mon::no);

            snd_emit::run(snd);

            mon->spell_cool_down_cur_ = mon->data().spell_cooldown_turns;
        }

        if (IS_INTRINSIC)
        {
            const Range cost = spi_cost(false, caster);

            caster->hit_spi(cost.roll(), Verbosity::silent);
        }

        Spell_effect_noticed is_noticed = Spell_effect_noticed::no;

        if (caster->is_alive())
        {
            is_noticed = cast_impl(caster);
        }

        game_time::tick();
        TRACE_FUNC_END;
        return is_noticed;
    }

    TRACE_FUNC_END;
    return Spell_effect_noticed::no;
}

void Spell::on_resist(Actor& target) const
{
    const bool IS_PLAYER        = target.is_player();
    const bool PLAYER_SEE_TGT   = map::player->can_see_actor(target);

    if (PLAYER_SEE_TGT)
    {
        msg_log::add(spell_resist_msg);

        if (IS_PLAYER)
        {
            audio::play(Sfx_id::spell_shield_break);
        }

        render::draw_blast_at_cells({target.pos}, clr_white);
    }

    if (IS_PLAYER)
    {
        target.prop_handler().end_prop(Prop_id::rSpell);
    }

}

//------------------------------------------------------------ DARKBOLT
Spell_effect_noticed Spell_darkbolt::cast_impl(Actor* const caster) const
{
    Actor* tgt = nullptr;

    std::vector<Actor*> seen_actors;
    caster->seen_foes(seen_actors);

    if (seen_actors.empty())
    {
        return Spell_effect_noticed::no;
    }

    tgt = map::random_closest_actor(caster->pos, seen_actors);

    //Spell reflection?
    if (tgt->has_prop(Prop_id::spell_reflect))
    {
        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(spell_reflect_msg,
                         clr_text,
                         false,
                         More_prompt_on_msg::yes);
        }

        return cast_impl(tgt);
    }

    //Spell resistance?
    if (tgt->has_prop(Prop_id::rSpell))
    {
        on_resist(*tgt);

        return Spell_effect_noticed::yes;
    }

    std::vector<P> line;

    line_calc::calc_new_line(caster->pos, tgt->pos, true, 999, false, line);

    render::draw_map_state();

    const size_t LINE_SIZE = line.size();

    for (size_t i = 1; i < LINE_SIZE; ++i)
    {
        const P& p = line[i];

        if (config::is_tiles_mode())
        {
            render::draw_tile(Tile_id::blast1, Panel::map, p, clr_magenta);
        }
        else
        {
            render::draw_glyph('*', Panel::map, p, clr_magenta);
        }

        render::update_screen();
        sdl_wrapper::sleep(config::delay_projectile_draw());
    }

    render::draw_blast_at_cells({tgt->pos}, clr_magenta);

    bool        is_warlock_charged  = false;
    Clr         msg_clr             = clr_msg_good;
    std::string str_begin           = "I am";

    if (tgt->is_player())
    {
        msg_clr = clr_msg_bad;
    }
    else //Target is monster
    {
        str_begin = tgt->name_the() + " is";

        if (map::player->is_leader_of(tgt))
        {
            msg_clr = clr_white;
        }
    }

    if (caster->is_player())
    {
        is_warlock_charged = caster->has_prop(Prop_id::warlock_charged);
    }

    if (map::player->can_see_actor(*tgt))
    {
        msg_log::add(str_begin + " struck by a blast!", msg_clr);
    }

    tgt->prop_handler().try_add(new Prop_paralyzed(Prop_turns::specific, 2));

    Range dmg_range(4, 10);
    const int DMG = is_warlock_charged ? dmg_range.max : dmg_range.roll();

    tgt->hit(DMG, Dmg_type::physical, Dmg_method::END, Allow_wound::no);

    Snd snd("",
            Sfx_id::END,
            Ignore_msg_if_origin_seen::yes,
            tgt->pos,
            nullptr,
            Snd_vol::low,
            Alerts_mon::yes);

    snd_emit::run(snd);

    return Spell_effect_noticed::yes;
}

bool Spell_darkbolt::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::one_in(3);
}

//------------------------------------------------------------ AZATHOTHS WRATH
Spell_effect_noticed Spell_aza_wrath::cast_impl(Actor* const caster) const
{
    Range dmg_range(4, 8);
    bool  is_warlock_charged = false;

    std::vector<Actor*> tgts;
    caster->seen_foes(tgts);

    if (tgts.empty())
    {
        return Spell_effect_noticed::no;
    }

    //This point reached means targets are available
    if (caster->is_player())
    {
        is_warlock_charged = caster->has_prop(Prop_id::warlock_charged);
    }

    render::draw_blast_at_seen_actors(tgts, clr_red_lgt);

    for (Actor* const tgt : tgts)
    {
        //Spell reflection?
        if (tgt->has_prop(Prop_id::spell_reflect))
        {
            if (map::player->can_see_actor(*tgt))
            {
                msg_log::add(spell_reflect_msg,
                             clr_white,
                             false,
                             More_prompt_on_msg::yes);
            }

            cast_impl(tgt);
            continue;
        }

        //Spell resistance?
        if (tgt->has_prop(Prop_id::rSpell))
        {
            on_resist(*tgt);

            continue;
        }

        std::string     str_begin   = "I am";
        Clr             msg_clr     = clr_msg_good;

        if (tgt->is_player())
        {
            msg_clr = clr_msg_bad;
        }
        else //Target is monster
        {
            str_begin = tgt->name_the() + " is";

            if (map::player->is_leader_of(tgt))
            {
                msg_clr = clr_white;
            }
        }

        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(str_begin + " struck by a roaring blast!", msg_clr);
        }

        tgt->prop_handler().try_add(new Prop_paralyzed(Prop_turns::specific, 2));

        const int DMG = is_warlock_charged ? dmg_range.max : dmg_range.roll();

        tgt->hit(DMG, Dmg_type::physical, Dmg_method::END, Allow_wound::no);

        Snd snd("",
                Sfx_id::END,
                Ignore_msg_if_origin_seen::yes,
                tgt->pos,
                nullptr,
                Snd_vol::high, Alerts_mon::yes);

        snd_emit::run(snd);
    }

    return Spell_effect_noticed::yes;
}

bool Spell_aza_wrath::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::one_in(3);
}

//------------------------------------------------------------ MAYHEM
Spell_effect_noticed Spell_mayhem::cast_impl(Actor* const caster) const
{
    const bool IS_PLAYER = caster->is_player();

    if (map::player->can_see_actor(*caster))
    {
        std::string caster_name = IS_PLAYER ? "me" : caster->name_the();
        msg_log::add("Destruction rages around " + caster_name + "!");
    }

    const P& caster_pos = caster->pos;

    const int NR_SWEEPS = 5;
    const int RADI      = fov_std_radi_int;

    const int X0 = std::max(1, caster_pos.x - RADI);
    const int Y0 = std::max(1, caster_pos.y - RADI);
    const int X1 = std::min(map_w - 1, caster_pos.x + RADI) - 1;
    const int Y1 = std::min(map_h - 1, caster_pos.y + RADI) - 1;

    for (int i = 0; i < NR_SWEEPS; ++i)
    {
        for (int y = Y0; y <= Y1; ++y)
        {
            for (int x = X0; x <= X1; ++x)
            {
                bool is_adj_to_walkable_cell = false;

                for (int dx = -1; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        const Rigid* const f = map::cells[x + dx][y + dy].rigid;

                        if (f->can_move_cmn()) {is_adj_to_walkable_cell = true;}
                    }
                }

                if (is_adj_to_walkable_cell && rnd::one_in(8))
                {
                    map::cells[x][y].rigid->hit(Dmg_type::physical, Dmg_method::explosion);
                }
            }
        }
    }

    for (int y = Y0; y <= Y1; ++y)
    {
        for (int x = X0; x <= X1; ++x)
        {
            auto* const f = map::cells[x][y].rigid;

            if (f->can_have_blood() && rnd::one_in(10))
            {
                f->mk_bloody();
            }

            if (rnd::one_in(4))
            {
                f->hit(Dmg_type::fire, Dmg_method::elemental, nullptr);
            }
        }
    }

    std::vector<Actor*> seen_foes;
    caster->seen_foes(seen_foes);

    for (auto* tgt : seen_foes)
    {
        //Spell reflection?
        if (tgt->has_prop(Prop_id::spell_reflect))
        {
            if (map::player->can_see_actor(*tgt))
            {
                msg_log::add(spell_reflect_msg,
                             clr_white,
                             false,
                             More_prompt_on_msg::yes);
            }

            cast_impl(tgt);
            continue;
        }

        //Spell resistance?
        if (tgt->has_prop(Prop_id::rSpell))
        {
            on_resist(*tgt);

            continue;
        }

        tgt->prop_handler().try_add(new Prop_burning(Prop_turns::std));
    }

    Snd snd("",
            Sfx_id::END,
            Ignore_msg_if_origin_seen::yes,
            caster_pos,
            nullptr,
            Snd_vol::high,
            Alerts_mon::yes);

    snd_emit::run(snd);

    return Spell_effect_noticed::yes;
}

bool Spell_mayhem::allow_mon_cast_now(Mon& mon) const
{
    return mon.aware_counter_ > 0 && rnd::coin_toss() && (mon.tgt_ || rnd::one_in(20));
}

//------------------------------------------------------------ PESTILENCE
Spell_effect_noticed Spell_pest::cast_impl(Actor* const caster) const
{
    const int RND = rnd::range(1, 3);

    const Actor_id monster_id = RND == 1 ? Actor_id::green_spider :
                                RND == 2 ? Actor_id::red_spider   :
                                Actor_id::rat;

    const size_t NR_MON = rnd::range(7, 10);

    Actor* leader = nullptr;

    bool did_player_summon_hostile = false;

    if (caster->is_player())
    {
        const int N = SUMMON_HOSTILE_ONE_IN_N *
                      (player_bon::traits[size_t(Trait::summoner)] ? 2 : 1);
        did_player_summon_hostile = rnd::one_in(N);

        leader = did_player_summon_hostile ? nullptr : caster;
    }
    else //Caster is monster
    {
        Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;

        leader = caster_leader ? caster_leader : caster;
    }

    std::vector<Mon*> mon_summoned;

    actor_factory::summon(caster->pos, {NR_MON, monster_id},
                          Make_mon_aware::yes,
                          leader,
                          &mon_summoned);

    bool is_any_seen_by_player = false;

    for (Mon* const mon : mon_summoned)
    {
        if (map::player->can_see_actor(*mon))
        {
            is_any_seen_by_player = true;
        }
    }

    if (is_any_seen_by_player)
    {
        std::string caster_str = "me";

        if (!caster->is_player())
        {
            if (map::player->can_see_actor(*caster))
            {
                caster_str = caster->name_the();
            }
            else
            {
                caster_str = "it";
            }
        }

        msg_log::add("Disgusting critters appear around " + caster_str + "!");

        if (did_player_summon_hostile)
        {
            msg_log::add("They are hostile!", clr_msg_note, true, More_prompt_on_msg::yes);
        }

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

bool Spell_pest::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_         &&
           rnd::one_in(3)   &&
           (mon.tgt_ || rnd::one_in(30));
}

//------------------------------------------------------------ ANIMATE WEAPONS
Spell_effect_noticed Spell_anim_wpns::cast_impl(Actor* const caster) const
{
    bool is_any_animated = false;

    if (caster->is_player())
    {
        for (int x = 0; x < map_w; ++x)
        {
            for (int y = 0; y < map_h; ++y)
            {
                Cell& cell = map::cells[x][y];

                Item* const item = cell.item;

                if (
                    cell.is_seen_by_player  &&
                    item                    &&
                    item->data().type == Item_type::melee_wpn)
                {
                    cell.item = nullptr;

                    const P p(x, y);

                    std::vector<Mon*> summoned;

                    actor_factory::summon(p,
                                          std::vector<Actor_id>(1, Actor_id::animated_wpn),
                                          Make_mon_aware::no,
                                          map::player,
                                          &summoned,
                                          Verbosity::silent);

                    ASSERT(summoned.size() == 1);

                    Mon* const anim_wpn = summoned[0];

                    Inventory& inv = anim_wpn->inv();

                    ASSERT(!inv.item_in_slot(Slot_id::wpn));

                    inv.put_in_slot(Slot_id::wpn, item);

                    const std::string item_name = item->name(Item_ref_type::plain,
                                                  Item_ref_inf::yes,
                                                  Item_ref_att_inf::none);

                    msg_log::add("The " + item_name + " rises into thin air!");

                    is_any_animated = true;
                }
            }
        }
    }

    return is_any_animated ?
           Spell_effect_noticed::yes : Spell_effect_noticed::no;
}

//------------------------------------------------------------ PHARAOH STAFF
Spell_effect_noticed Spell_pharaoh_staff::cast_impl(Actor* const caster) const
{
    //First check for a friendly mummy and heal it (as per the spell description)
    for (Actor* const actor : game_time::actors)
    {
        const auto actor_id = actor->data().id;

        const bool IS_ACTOR_ID_OK = actor_id == Actor_id::mummy ||
                                    actor_id == Actor_id::croc_head_mummy;

        if (IS_ACTOR_ID_OK && caster->is_leader_of(actor))
        {
            actor->restore_hp(999);
            return Spell_effect_noticed::yes;
        }
    }

    //This point reached means no mummy controlled, summon a new one
    Actor* leader = nullptr;

    bool did_player_summon_hostile = false;

    if (caster->is_player())
    {
        const int N = SUMMON_HOSTILE_ONE_IN_N *
                      (player_bon::traits[size_t(Trait::summoner)] ? 2 : 1);

        did_player_summon_hostile = rnd::one_in(N);
        leader = did_player_summon_hostile ? nullptr : caster;
    }
    else //Caster is monster
    {
        Actor* const caster_leader = static_cast<Mon*>(caster)->leader_;
        leader = caster_leader ? caster_leader : caster;
    }

    std::vector<Mon*> summoned_mon;

    const auto actor_id = rnd::coin_toss() ? Actor_id::mummy : Actor_id::croc_head_mummy;

    actor_factory::summon(caster->pos, {actor_id},
                          Make_mon_aware::yes,
                          leader,
                          &summoned_mon);

    const Mon* const mon = summoned_mon[0];

    if (map::player->can_see_actor(*mon))
    {
        msg_log::add(mon->name_a() + " appears!");

        if (did_player_summon_hostile)
        {
            msg_log::add("It is hostile!",
                         clr_msg_note,
                         true,
                         More_prompt_on_msg::yes);
        }

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

bool Spell_pharaoh_staff::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::one_in(4);
}

//------------------------------------------------------------ DETECT ITEMS
Spell_effect_noticed Spell_det_items::cast_impl(Actor* const caster) const
{
    (void)caster;

    const int RADI    = fov_std_radi_int + 3;
    const int ORIG_X  = map::player->pos.x;
    const int ORIG_Y  = map::player->pos.y;
    const int X0      = std::max(0, ORIG_X - RADI);
    const int Y0      = std::max(0, ORIG_Y - RADI);
    const int X1      = std::min(map_w - 1, ORIG_X + RADI);
    const int Y1      = std::min(map_h - 1, ORIG_Y + RADI);

    std::vector<P> items_revealed_cells;

    for (int y = Y0; y < Y1; ++y)
    {
        for (int x = X0; x <= X1; ++x)
        {
            Item* item = map::cells[x][y].item;

            if (item)
            {
                map::cells[x][y].is_seen_by_player = true;
                map::cells[x][y].is_explored     = true;
                items_revealed_cells.push_back(P(x, y));
            }
        }
    }

    if (!items_revealed_cells.empty())
    {
        render::draw_map_state();
        map::player->update_fov();
        render::draw_blast_at_cells(items_revealed_cells, clr_white);
        render::draw_map_state();

        if (items_revealed_cells.size() == 1)
        {
            msg_log::add("An item is revealed to me.");
        }

        if (items_revealed_cells.size() > 1)
        {
            msg_log::add("Some items are revealed to me.");
        }

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

//------------------------------------------------------------ DETECT TRAPS
Spell_effect_noticed Spell_det_traps::cast_impl(Actor* const caster) const
{
    (void)caster;

    std::vector<P> traps_revealed_cells;

    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            if (map::cells[x][y].is_seen_by_player)
            {
                auto* const f = map::cells[x][y].rigid;

                if (f->id() == Feature_id::trap)
                {
                    auto* const trap = static_cast<Trap*>(f);
                    trap->reveal(false);
                    traps_revealed_cells.push_back(P(x, y));
                }
            }
        }
    }

    if (!traps_revealed_cells.empty())
    {
        render::draw_map_state();
        map::player->update_fov();
        render::draw_blast_at_cells(traps_revealed_cells, clr_white);
        render::draw_map_state();

        if (traps_revealed_cells.size() == 1)
        {
            msg_log::add("A hidden trap is revealed to me.");
        }

        if (traps_revealed_cells.size() > 1)
        {
            msg_log::add("Some hidden traps are revealed to me.");
        }

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

//------------------------------------------------------------ DETECT MONSTERS
Spell_effect_noticed Spell_det_mon::cast_impl(Actor* const caster) const
{
    (void)caster;

    bool is_seer = player_bon::traits[size_t(Trait::seer)];

    const int MULTIPLIER  = 6 * (is_seer ? 3 : 1);

    Spell_effect_noticed is_noticed = Spell_effect_noticed::no;

    for (Actor* actor : game_time::actors)
    {
        if (!actor->is_player())
        {
            static_cast<Mon*>(actor)->set_player_aware_of_me(MULTIPLIER);
            is_noticed = Spell_effect_noticed::yes;
        }
    }

    if (is_noticed == Spell_effect_noticed::yes)
    {
        msg_log::add("I detect creatures.");
    }

    return is_noticed;
}

//------------------------------------------------------------ OPENING
Spell_effect_noticed Spell_opening::cast_impl(Actor* const caster) const
{
    (void)caster;

    bool is_any_opened = false;

    for (int y = 1; y < map_h - 1; ++y)
    {
        for (int x = 1; x < map_w - 1; ++x)
        {
            const auto& cell = map::cells[x][y];

            if (cell.is_seen_by_player)
            {
                Did_open did_open = cell.rigid->open(nullptr);

                if (did_open == Did_open::yes)
                {
                    is_any_opened = true;
                }
            }
        }
    }

    if (!is_any_opened)
    {
        return Spell_effect_noticed::no;
    }

    map::player->update_fov();
    render::draw_map_state();
    return Spell_effect_noticed::yes;
}

//------------------------------------------------------------ SACRIFICE LIFE
Spell_effect_noticed Spell_sacr_life::cast_impl(Actor* const caster) const
{
    (void)caster;

    //Convert every 2 HP to 1 SPI

    const int PLAYER_HP_CUR = map::player->hp();

    if (PLAYER_HP_CUR > 2)
    {
        const int HP_DRAINED = ((PLAYER_HP_CUR - 1) / 2) * 2;

        map::player->hit(HP_DRAINED, Dmg_type::pure);

        map::player->restore_spi(HP_DRAINED, true);

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

//------------------------------------------------------------ SACRIFICE SPIRIT
Spell_effect_noticed Spell_sacr_spi::cast_impl(Actor* const caster) const
{
    (void)caster;

    //Convert all SPI to HP

    const int PLAYER_SPI_CUR = map::player->spi();

    if (PLAYER_SPI_CUR > 0)
    {
        const int HP_DRAINED = PLAYER_SPI_CUR - 1;

        map::player->hit_spi(HP_DRAINED);

        map::player->restore_hp(HP_DRAINED);

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

//------------------------------------------------------------ ROGUE CLOUD MINDS
Spell_effect_noticed Spell_cloud_minds::cast_impl(Actor* const caster) const
{
    (void)caster;

    msg_log::add("I vanish from the minds of my enemies.");

    for (Actor* actor : game_time::actors)
    {
        if (!actor->is_player())
        {
            Mon* const mon = static_cast<Mon*>(actor);
            mon->aware_counter_ = 0;
        }
    }

    return Spell_effect_noticed::yes;
}

//------------------------------------------------------------ GHOUL FRENZY
Spell_effect_noticed Spell_frenzy::cast_impl(Actor* const caster) const
{
    const int NR_TURNS = rnd::range(12, 18);

    Prop_frenzied* frenzy = new Prop_frenzied(Prop_turns::specific, NR_TURNS);

    caster->prop_handler().try_add(frenzy);

    return Spell_effect_noticed::yes;
}

//------------------------------------------------------------ BLESS
Spell_effect_noticed Spell_bless::cast_impl(Actor* const caster) const
{
    caster->prop_handler().try_add(new Prop_blessed(Prop_turns::std));

    return Spell_effect_noticed::yes;
}

//------------------------------------------------------------ LIGHT
Spell_effect_noticed Spell_light::cast_impl(Actor* const caster) const
{
    caster->prop_handler().try_add(new Prop_radiant(Prop_turns::std));
    return Spell_effect_noticed::yes;
}

//------------------------------------------------------------ TELEPORT
Spell_effect_noticed Spell_teleport::cast_impl(Actor* const caster) const
{
    caster->teleport();

    return Spell_effect_noticed::yes;
}

bool Spell_teleport::allow_mon_cast_now(Mon& mon) const
{
    const bool IS_LOW_HP = mon.hp() <= (mon.hp_max(true) / 2);

    return (mon.aware_counter_ > 0) && IS_LOW_HP && rnd::fraction(3, 4);
}

//------------------------------------------------------------ RESISTANCE
Spell_effect_noticed Spell_res::cast_impl(Actor* const caster) const
{
    const int DURATION = 20;

    Prop_handler& prop_hlr = caster->prop_handler();

    prop_hlr.try_add(new Prop_rFire(Prop_turns::specific, DURATION));
    prop_hlr.try_add(new Prop_rElec(Prop_turns::specific, DURATION));

    return Spell_effect_noticed::yes;
}

bool Spell_res::allow_mon_cast_now(Mon& mon) const
{
    const bool HAS_RFIRE = mon.prop_handler().has_prop(Prop_id::rFire);
    const bool HAS_RELEC = mon.prop_handler().has_prop(Prop_id::rElec);

    return (!HAS_RFIRE || !HAS_RELEC) && mon.tgt_ && rnd::coin_toss();
}

//------------------------------------------------------------ KNOCKBACK
Spell_effect_noticed Spell_knock_back::cast_impl(Actor* const caster) const
{
    ASSERT(!caster->is_player());

    Clr             msg_clr     = clr_msg_good;
    std::string     tgt_str     = "me";
    Actor*          caster_used = caster;
    Actor*          tgt         = static_cast<Mon*>(caster_used)->tgt_;
    ASSERT(tgt);

    //Spell reflection?
    if (tgt->has_prop(Prop_id::spell_reflect))
    {
        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(spell_reflect_msg,
                         clr_text,
                         false,
                         More_prompt_on_msg::yes);
        }

        std::swap(caster_used, tgt);
    }

    //Spell resistance?
    if (tgt->has_prop(Prop_id::rSpell))
    {
        on_resist(*tgt);

        return Spell_effect_noticed::yes;
    }

    if (tgt->is_player())
    {
        msg_clr = clr_msg_bad;
    }
    else //Target is monster
    {
        tgt_str = tgt->name_the();

        if (map::player->is_leader_of(tgt)) {msg_clr = clr_white;}
    }

    if (map::player->can_see_actor(*tgt))
    {
        msg_log::add("A force pushes " + tgt_str + "!", msg_clr);
    }

    knock_back::try_knock_back(*tgt, caster->pos, false);

    return Spell_effect_noticed::no;
}

bool Spell_knock_back::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::coin_toss();
}

//------------------------------------------------------------ PROP ON OTHERS
Spell_effect_noticed Spell_prop_on_mon::cast_impl(Actor* const caster) const
{
    const Prop_id prop_id = applied_prop_id();

    std::vector<Actor*> tgts;
    caster->seen_foes(tgts);

    if (tgts.empty())
    {
        return Spell_effect_noticed::no;
    }

    render::draw_blast_at_seen_actors(tgts, clr_magenta);

    for (Actor* const tgt : tgts)
    {
        Prop_handler& prop_handler = tgt->prop_handler();

        //Spell reflection?
        if (tgt->has_prop(Prop_id::spell_reflect))
        {
            if (map::player->can_see_actor(*tgt))
            {
                msg_log::add(spell_reflect_msg,
                             clr_text,
                             false,
                             More_prompt_on_msg::yes);
            }

            cast_impl(tgt);
            continue;
        }

        //Spell resistance?
        if (tgt->has_prop(Prop_id::rSpell))
        {
            on_resist(*tgt);

            continue;
        }

        Prop* const prop = prop_handler.mk_prop(prop_id, Prop_turns::std);
        prop_handler.try_add(prop);
    }

    return Spell_effect_noticed::yes;
}

bool Spell_prop_on_mon::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_;
}

bool Spell_paralyze_mon::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::one_in(4);
}

//------------------------------------------------------------ DISEASE
Spell_effect_noticed Spell_disease::cast_impl(Actor* const caster) const
{
    ASSERT(!caster->is_player());

    Actor* caster_used = caster;
    Actor* tgt        = static_cast<Mon*>(caster_used)->tgt_;

    //Spell reflection?
    if (tgt->has_prop(Prop_id::spell_reflect))
    {
        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(spell_reflect_msg,
                         clr_text,
                         false,
                         More_prompt_on_msg::yes);
        }

        std::swap(caster_used, tgt);
    }

    //Spell resistance?
    if (tgt->has_prop(Prop_id::rSpell))
    {
        on_resist(*tgt);

        return Spell_effect_noticed::yes;
    }

    std::string actor_name = "me";

    if (!tgt->is_player())
    {
        actor_name = tgt->name_the();
    }

    if (map::player->can_see_actor(*tgt))
    {
        msg_log::add("A horrible disease is starting to afflict " + actor_name + "!");
    }

    tgt->prop_handler().try_add(new Prop_diseased(Prop_turns::std));
    return Spell_effect_noticed::no;
}

bool Spell_disease::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::coin_toss();
}

//------------------------------------------------------------ SUMMON MONSTER
Spell_effect_noticed Spell_summon_mon::cast_impl(Actor* const caster) const
{
    //Try to summon a creature inside the player's FOV (inside the standard range), in a
    //free visible cell. If no such cell is available, instead summon near the caster.

    bool blocked[map_w][map_h];
    map_parse::run(cell_check::Blocks_move_cmn(true), blocked);

    std::vector<P> free_cells_seen_by_player;
    const int RADI = fov_std_radi_int;
    const P player_pos(map::player->pos);
    const int X0 = std::max(0, player_pos.x - RADI);
    const int Y0 = std::max(0, player_pos.y - RADI);
    const int X1 = std::min(map_w, player_pos.x + RADI) - 1;
    const int Y1 = std::min(map_h, player_pos.y + RADI) - 1;

    for (int x = X0; x <= X1; ++x)
    {
        for (int y = Y0; y <= Y1; ++y)
        {
            if (!blocked[x][y] && map::cells[x][y].is_seen_by_player)
            {
                free_cells_seen_by_player.push_back(P(x, y));
            }
        }
    }

    P summon_pos(-1, -1);

    if (free_cells_seen_by_player.empty())
    {
        //No free cells seen by player, instead summon near the caster.
        std::vector<P> free_cells_vector;
        to_vec((bool*)blocked, false, map_w, map_h, free_cells_vector);

        if (!free_cells_vector.empty())
        {
            sort(free_cells_vector.begin(), free_cells_vector.end(),
                 Is_closer_to_pos(caster->pos));
            summon_pos = free_cells_vector[0];
        }
    }
    else //There are free cells seen by the player available
    {
        const size_t IDX    = rnd::range(0, free_cells_seen_by_player.size() - 1);
        summon_pos          = free_cells_seen_by_player[IDX];
    }

    std::vector<Actor_id> summon_bucket;

    for (int i = 0; i < int(Actor_id::END); ++i)
    {
        const Actor_data_t& data = actor_data::data[i];

        if (data.can_be_summoned)
        {
            //Method for finding eligible monsters depends on if player or monster is casting.
            int dlvl_max = -1;

            if (caster->is_player())
            {
                //Compare player CVL with monster's allowed spawning DLVL.
                const int PLAYER_CLVL     = dungeon_master::clvl();
                const int PLAYER_CLVL_PCT = (PLAYER_CLVL * 100) / player_max_clvl;

                dlvl_max                   = (PLAYER_CLVL_PCT * dlvl_last) / 100;
            }
            else //Caster is monster
            {
                //Compare caster and summoned monster's allowed spawning DLVL.
                dlvl_max = caster->data().spawn_min_dlvl;
            }

            if (data.spawn_min_dlvl <= dlvl_max)
            {
                summon_bucket.push_back(Actor_id(i));
            }
        }
    }

    if (summon_bucket.empty())
    {
        TRACE << "No elligible monsters found for spawning" << std::endl;
        ASSERT(false);
        return Spell_effect_noticed::no;
    }

    const int       IDX                         = rnd::range(0, summon_bucket.size() - 1);
    const Actor_id  mon_id                      = summon_bucket[IDX];
    Actor*          leader                      = nullptr;
    bool            did_player_summon_hostile   = false;

    if (caster->is_player())
    {
        const int N                 = SUMMON_HOSTILE_ONE_IN_N *
                                      (player_bon::traits[size_t(Trait::summoner)] ? 2 : 1);
        did_player_summon_hostile   = rnd::one_in(N);
        leader                      = did_player_summon_hostile ? nullptr : caster;
    }
    else //Caster is monster
    {
        Actor* const caster_leader  = static_cast<Mon*>(caster)->leader_;
        leader                      = caster_leader ? caster_leader : caster;
    }

    std::vector<Mon*> mon_summoned;

    actor_factory::summon(summon_pos, {mon_id},
                          Make_mon_aware::yes,
                          leader,
                          &mon_summoned);

    Mon* const mon = mon_summoned[0];

    if (map::player->can_see_actor(*mon))
    {
        msg_log::add(mon->name_a() + " appears!");

        if (did_player_summon_hostile)
        {
            msg_log::add("It is hostile!",
                         clr_msg_note,
                         true,
                         More_prompt_on_msg::yes);
        }

        return Spell_effect_noticed::yes;
    }

    return Spell_effect_noticed::no;
}

bool Spell_summon_mon::allow_mon_cast_now(Mon& mon) const
{
    //NOTE: Checking awareness instead of target, to allow summoning even with broken LOS
    return (mon.aware_counter_ > 0) &&
           rnd::coin_toss()         &&
           (mon.tgt_ || rnd::one_in(23));
}

//------------------------------------------------------------ HEAL SELF
Spell_effect_noticed Spell_heal_self::cast_impl(Actor* const caster) const
{
    //The spell effect is noticed if any hit points were restored
    const bool IS_ANY_HP_HEALED = caster->restore_hp(999);

    return IS_ANY_HP_HEALED ? Spell_effect_noticed::yes : Spell_effect_noticed::no;
}

bool Spell_heal_self::allow_mon_cast_now(Mon& mon) const
{
    return mon.hp() < mon.hp_max(true);
}

//------------------------------------------------------------ MI-GO HYPNOSIS
Spell_effect_noticed Spell_mi_go_hypno::cast_impl(Actor* const caster) const
{
    ASSERT(!caster->is_player());

    Actor* caster_used  = caster;
    Actor* tgt          = static_cast<Mon*>(caster_used)->tgt_;

    ASSERT(tgt);

    //Spell reflection?
    if (tgt->has_prop(Prop_id::spell_reflect))
    {
        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(spell_reflect_msg,
                         clr_text,
                         false,
                         More_prompt_on_msg::yes);
        }

        std::swap(caster_used, tgt);
    }

    //Spell resistance?
    if (tgt->has_prop(Prop_id::rSpell))
    {
        on_resist(*tgt);

        return Spell_effect_noticed::yes;
    }

    if (tgt->is_player())
    {
        msg_log::add("There is a sharp droning in my head!");
    }

    if (rnd::coin_toss())
    {
        Prop* const prop = new Prop_fainted(Prop_turns::specific, rnd::range(2, 10));
        tgt->prop_handler().try_add(prop);
    }
    else
    {
        msg_log::add("I feel dizzy.");
    }

    return Spell_effect_noticed::yes;
}

bool Spell_mi_go_hypno::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_                 &&
           mon.tgt_->is_player()    &&
           rnd::one_in(3);
}

//------------------------------------------------------------ IMMOLATION
Spell_effect_noticed Spell_burn::cast_impl(Actor* const caster) const
{
    ASSERT(!caster->is_player());

    Actor* caster_used = caster;
    Actor* tgt        = static_cast<Mon*>(caster_used)->tgt_;

    ASSERT(tgt);

    //Spell reflection?
    if (tgt->has_prop(Prop_id::spell_reflect))
    {
        if (map::player->can_see_actor(*tgt))
        {
            msg_log::add(spell_reflect_msg,
                         clr_text,
                         false,
                         More_prompt_on_msg::yes);
        }

        std::swap(caster_used, tgt);
    }

    //Spell resistance?
    if (tgt->has_prop(Prop_id::rSpell))
    {
        on_resist(*tgt);

        return Spell_effect_noticed::yes;
    }

    std::string tgt_str = "me";

    if (!tgt->is_player())
    {
        tgt_str = tgt->name_the();
    }

    if (map::player->can_see_actor(*tgt))
    {
        msg_log::add("Flames are rising around " + tgt_str + "!");
    }

    Prop* const prop = new Prop_burning(Prop_turns::specific, rnd::range(3, 4));
    tgt->prop_handler().try_add(prop);

    return Spell_effect_noticed::yes;
}

bool Spell_burn::allow_mon_cast_now(Mon& mon) const
{
    return mon.tgt_ && rnd::one_in(4);
}
