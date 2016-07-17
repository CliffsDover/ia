#include "insanity.hpp"

#include "popup.hpp"
#include "dungeon_master.hpp"
#include "sound.hpp"
#include "map.hpp"
#include "actor_player.hpp"
#include "msg_log.hpp"
#include "actor_mon.hpp"
#include "properties.hpp"
#include "feature_rigid.hpp"
#include "actor_factory.hpp"
#include "save_handling.hpp"
#include "init.hpp"
#include "game_time.hpp"
#include "render.hpp"

//-----------------------------------------------------------------------------
// Insanity symptoms
//-----------------------------------------------------------------------------
void Ins_sympt::on_start()
{
    msg_log::more_prompt();

    const std::string heading   = start_heading();
    const std::string msg       = "Insanity draws nearer... " + start_msg();

    ASSERT(!heading.empty() && !msg.empty());

    if (!heading.empty() && !msg.empty())
    {
        popup::show_msg(msg, true, heading, Sfx_id::insanity_rise);
    }

    const std::string history_event_msg = history_msg();

    ASSERT(!history_event_msg.empty());

    if (!history_event_msg.empty())
    {
        dungeon_master::add_history_event(history_event_msg);
    }

    on_start_hook();
}

void Ins_sympt::on_end()
{
    const std::string msg = end_msg();

    ASSERT(!msg.empty());

    if (!msg.empty())
    {
        msg_log::add(msg);
    }

    const std::string history_event_msg = history_msg_end();

    ASSERT(!history_event_msg.empty());

    if (!history_event_msg.empty())
    {
        dungeon_master::add_history_event(history_event_msg);
    }
}

bool Ins_scream::is_allowed() const
{
    return !map::player->has_prop(Prop_id::rFear);
}

void Ins_scream::on_start_hook()
{
    Snd snd("",
            Sfx_id::END,
            Ignore_msg_if_origin_seen::yes,
            map::player->pos,
            map::player,
            Snd_vol::high,
            Alerts_mon::yes);

    snd_emit::run(snd);
}

std::string Ins_scream::start_msg() const
{
    if (rnd::coin_toss())
    {
        return "I let out a terrified shriek.";
    }
    else
    {
        return "I scream in terror.";
    }
}

void Ins_babbling::on_start_hook()
{
    const std::string player_name = map::player->name_the();

    for (int i = rnd::range(3, 5); i > 0; --i)
    {
        msg_log::add(player_name + ": " + Cultist::cultist_phrase());
    }

    Snd snd("",
            Sfx_id::END,
            Ignore_msg_if_origin_seen::yes,
            map::player->pos,
            map::player,
            Snd_vol::low,
            Alerts_mon::yes);

    snd_emit::run(snd);
}

void Ins_babbling::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    (void)seen_foes;
}

bool Ins_faint::is_allowed() const
{
    return true;
}

void Ins_faint::on_start_hook()
{
    map::player->prop_handler().try_add(new Prop_fainted(Prop_turns::std));
}

void Ins_laugh::on_start_hook()
{
    Snd snd("",
            Sfx_id::END,
            Ignore_msg_if_origin_seen::yes,
            map::player->pos,
            map::player,
            Snd_vol::low,
            Alerts_mon::yes);

    snd_emit::run(snd);
}

bool Ins_phobia_rat::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_rat::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    if (rnd::one_in(10))
    {
        for (Actor* const actor : seen_foes)
        {
            if (actor->data().is_rat)
            {
                msg_log::add("I am plagued by my phobia of rats!");

                map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));

                break;
            }
        }
    }
}

void Ins_phobia_rat::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_spider::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_spider::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    if (rnd::one_in(10))
    {
        for (Actor* const actor : seen_foes)
        {
            if (actor->data().is_spider)
            {
                msg_log::add("I am plagued by my phobia of spiders!");

                map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));

                break;
            }
        }
    }
}

void Ins_phobia_spider::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_reptile_and_amph::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_reptile_and_amph::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    bool is_triggered = false;

    if (rnd::one_in(10))
    {
        for (Actor* const actor : seen_foes)
        {
            if (actor->data().is_reptile)
            {
                msg_log::add("I am plagued by my phobia of reptiles!");
                is_triggered = true;
                break;
            }

            if (actor->data().is_amphibian)
            {
                msg_log::add("I am plagued by my phobia of amphibians!");
                is_triggered = true;
                break;
            }
        }
    }

    if (is_triggered)
    {
        map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));
    }
}

void Ins_phobia_reptile_and_amph::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_canine::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_canine::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    if (rnd::one_in(10))
    {
        for (Actor* const actor : seen_foes)
        {
            if (actor->data().is_canine)
            {
                msg_log::add("I am plagued by my phobia of canines!");

                map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));

                break;
            }
        }
    }
}

void Ins_phobia_canine::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_dead::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_dead::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    if (rnd::one_in(10))
    {
        for (Actor* const actor : seen_foes)
        {
            if (actor->data().is_undead)
            {
                msg_log::add("I am plagued by my phobia of the dead!");

                map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));

                break;
            }
        }
    }
}

void Ins_phobia_dead::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_open::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_open::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    (void)seen_foes;

    if (rnd::one_in(10) && map::player->is_standing_in_open_place())
    {
        msg_log::add("I am plagued by my phobia of open places!");

        map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));
    }
}

void Ins_phobia_open::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_confined::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_confined::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    (void)seen_foes;

    if (rnd::one_in(10) && map::player->is_standing_in_cramped_place())
    {
        msg_log::add("I am plagued by my phobia of confined places!");

        map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));
    }
}

void Ins_phobia_confined::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_deep::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_deep::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    (void)seen_foes;

    if (rnd::one_in(10))
    {
        for (const P& d : dir_utils::dir_list)
        {
            const P p(map::player->pos + d);

            if (map::cells[p.x][p.y].rigid->is_bottomless())
            {
                msg_log::add("I am plagued by my phobia of deep places!");

                map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));

                break;
            }
        }
    }
}

void Ins_phobia_deep::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_phobia_dark::is_allowed() const
{
    const bool has_phobia   = insanity::has_sympt_type(Ins_sympt_type::phobia);
    const bool is_rfear     = map::player->has_prop(Prop_id::rFear);

    return !is_rfear && (!has_phobia || rnd::one_in(20));
}

void Ins_phobia_dark::on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    (void)seen_foes;

    if (rnd::one_in(10))
    {
        const P             p(map::player->pos);
        const Prop_handler& props = map::player->prop_handler();

        if (
            (props.allow_act() && !props.allow_see()) ||
            (map::cells[p.x][p.y].is_dark && !map::cells[p.x][p.y].is_lit))
        {
            msg_log::add("I am plagued by my phobia of the dark!");

            map::player->prop_handler().try_add(new Prop_terrified(Prop_turns::std));
        }
    }
}

void Ins_phobia_dark::on_permanent_rfear()
{
    insanity::end_sympt(id());
}

bool Ins_masoch::is_allowed() const
{
    const bool is_sadist = insanity::has_sympt(Ins_sympt_id::sadism);

    return !is_sadist && rnd::one_in(10);
}

bool Ins_sadism::is_allowed() const
{
    const bool is_masoch = insanity::has_sympt(Ins_sympt_id::masoch);

    return !is_masoch && rnd::one_in(4);
}

void Ins_shadows::on_start_hook()
{
    TRACE_FUNC_BEGIN;

    const int nr_shadows_lower = 2;

    const int nr_shadows_upper = constr_in_range(nr_shadows_lower, map::dlvl - 2, 8);

    const int nr = rnd::range(nr_shadows_lower, nr_shadows_upper);

    std::vector<Actor_id> shadow_ids(nr, Actor_id::shadow);

    std::vector<Mon*> summoned;

    actor_factory::summon(map::player->pos,
                          shadow_ids,
                          Make_mon_aware::yes,
                          nullptr,
                          &summoned,
                          Verbosity::silent);

    ASSERT(!summoned.empty());

    for (Mon* const mon : summoned)
    {
        mon->is_sneaking_                   = true;
        mon->player_aware_of_me_counter_    = 0;

        auto* const disabled_att = new Prop_disabled_attack(Prop_turns::specific, 1);

        mon->prop_handler().try_add(disabled_att);
    }

    map::player->update_fov();

    std::vector<Actor*> player_seen_foes;
    map::player->seen_foes(player_seen_foes);

    for (Actor* const actor : player_seen_foes)
    {
        static_cast<Mon*>(actor)->set_player_aware_of_me();
    }

    render::draw_map_state();

    map::cpy_render_array_to_visual_memory();

    TRACE_FUNC_END;
}

void Ins_paranoia::on_start_hook()
{
    //Flip a coint to decide if we should spawn a stalker or not
    //(Maybe it's just paranoia, or maybe it's real)
    if (rnd::coin_toss())
    {
        std::vector<Actor_id> stalker_id(1, Actor_id::invis_stalker);

        std::vector<Mon*> summoned;

        actor_factory::summon(map::player->pos,
                              stalker_id,
                              Make_mon_aware::yes,
                              nullptr,
                              &summoned,
                              Verbosity::silent);

        ASSERT(summoned.size() == 1);

        Mon* const mon = summoned[0];

        auto* const disabled_att = new Prop_disabled_attack(Prop_turns::specific, 2);

        mon->prop_handler().try_add(disabled_att);
    }
}

bool Ins_confusion::is_allowed() const
{
    return !map::player->has_prop(Prop_id::rConf);
}

void Ins_confusion::on_start_hook()
{
    map::player->prop_handler().try_add(new Prop_confused(Prop_turns::std));
}

bool Ins_frenzy::is_allowed() const
{
    return true;
}

void Ins_frenzy::on_start_hook()
{
    map::player->prop_handler().try_add(new Prop_frenzied(Prop_turns::std));
}


//-----------------------------------------------------------------------------
// Insanity handling
//-----------------------------------------------------------------------------
namespace insanity
{

namespace
{

Ins_sympt* sympts_[size_t(Ins_sympt_id::END)];

Ins_sympt* mk_sympt(const Ins_sympt_id id)
{
    switch (id)
    {
    case Ins_sympt_id::scream:
        return new Ins_scream();

    case Ins_sympt_id::babbling:
        return new Ins_babbling();

    case Ins_sympt_id::faint:
        return new Ins_faint();

    case Ins_sympt_id::laugh:
        return new Ins_laugh();

    case Ins_sympt_id::phobia_rat:
        return new Ins_phobia_rat();

    case Ins_sympt_id::phobia_spider:
        return new Ins_phobia_spider();

    case Ins_sympt_id::phobia_reptile_and_amph:
        return new Ins_phobia_reptile_and_amph();

    case Ins_sympt_id::phobia_canine:
        return new Ins_phobia_canine();

    case Ins_sympt_id::phobia_dead:
        return new Ins_phobia_dead();

    case Ins_sympt_id::phobia_open:
        return new Ins_phobia_open();

    case Ins_sympt_id::phobia_confined:
        return new Ins_phobia_confined();

    case Ins_sympt_id::phobia_deep:
        return new Ins_phobia_deep();

    case Ins_sympt_id::phobia_dark:
        return new Ins_phobia_dark();

    case Ins_sympt_id::masoch:
        return new Ins_masoch();

    case Ins_sympt_id::sadism:
        return new Ins_sadism();

    case Ins_sympt_id::shadows:
        return new Ins_shadows();

    case Ins_sympt_id::paranoia:
        return new Ins_paranoia();

    case Ins_sympt_id::confusion:
        return new Ins_confusion();

    case Ins_sympt_id::frenzy:
        return new Ins_frenzy();

    case Ins_sympt_id::strange_sensation:
        return new Ins_strange_sensation();

    case Ins_sympt_id::END:
        break;
    }

    ASSERT(false);

    return nullptr;
}

} //namespace


void init()
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        sympts_[i] = nullptr;
    }
}

void cleanup()
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        delete sympts_[i];

        sympts_[i] = nullptr;
    }
}

void save()
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        const Ins_sympt* const sympt = sympts_[i];

        save_handling::put_bool(sympt);

        if (sympt)
        {
            sympt->save();
        }
    }
}

void load()
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        const bool has_symptom = save_handling::get_bool();

        if (has_symptom)
        {
            Ins_sympt* const sympt = mk_sympt(Ins_sympt_id(i));

            sympts_[i] = sympt;

            sympt->load();
        }
    }
}

void run_sympt()
{
    std::vector<Ins_sympt*> sympt_bucket;

    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        const Ins_sympt* const active_sympt = sympts_[i];

        //Symptoms are only allowed if not already active
        if (!active_sympt)
        {
            Ins_sympt* const new_sympt = mk_sympt(Ins_sympt_id(i));

            const bool is_allowed = new_sympt->is_allowed();

            if (is_allowed)
            {
                sympt_bucket.push_back(new_sympt);
            }
            else //Not allowed (e.g. player resistant to fear, and this is a phobia)
            {
                delete new_sympt;
            }
        }
    }

    if (sympt_bucket.empty())
    {
        //This should never be able to happen, since there are symptoms which can occur repeatedly,
        //unconditionally - but we do a check anyway for robustness.
        return;
    }

    const size_t bucket_idx = rnd::range(0, sympt_bucket.size() - 1);

    Ins_sympt* const sympt = sympt_bucket[bucket_idx];

    sympt_bucket.erase(begin(sympt_bucket) + bucket_idx);

    //Delete the remaining symptoms in the bucket
    for (auto* const sympt : sympt_bucket)
    {
        delete sympt;
    }

    //If the symptom is permanent (i.e. not just a one-shot thing like screaming), set it as active
    //in the symptoms list
    if (sympt->is_permanent())
    {
        const size_t sympt_idx = size_t(sympt->id());

        ASSERT(!sympts_[sympt_idx]);

        sympts_[sympt_idx] = sympt;
    }

    sympt->on_start();
}

bool has_sympt(const Ins_sympt_id id)
{
    ASSERT(id != Ins_sympt_id::END);

    return sympts_[size_t(id)];
}

bool has_sympt_type(const Ins_sympt_type type)
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        const Ins_sympt* const s = sympts_[i];

        if (s && s->type() == type)
        {
            return true;
        }
    }

    return false;
}

std::vector<const Ins_sympt*> active_sympts()
{
    std::vector<const Ins_sympt*> out;

    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        const Ins_sympt* const sympt = sympts_[i];

        if (sympt)
        {
            out.push_back(sympt);
        }
    }

    return out;
}

void on_new_player_turn(const std::vector<Actor*>& seen_foes)
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        Ins_sympt* const sympt = sympts_[i];

        if (sympt)
        {
            sympt->on_new_player_turn(seen_foes);
        }
    }
}

void on_permanent_rfear()
{
    for (size_t i = 0; i < size_t(Ins_sympt_id::END); ++i)
    {
        Ins_sympt* const sympt = sympts_[i];

        if (sympt)
        {
            sympt->on_permanent_rfear();
        }
    }
}

void end_sympt(const Ins_sympt_id id)
{
    ASSERT(id != Ins_sympt_id::END);

    const size_t idx = size_t(id);

    Ins_sympt* const sympt = sympts_[idx];

    ASSERT(sympt);

    sympts_[idx] = nullptr;

    sympt->on_end();

    delete sympt;
}

} //insanity
