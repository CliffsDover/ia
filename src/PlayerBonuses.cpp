#include "PlayerBonuses.h"

#include <assert.h>

#include "Engine.h"
#include "TextFormatting.h"
#include "ActorPlayer.h"
#include "DungeonMaster.h"
#include "ItemScroll.h"
#include "ItemPotion.h"
#include "ItemFactory.h"
#include "Inventory.h"

using namespace std;

PlayerBonHandler::PlayerBonHandler(Engine& engine) :
  bg_(endOfBgs), eng(engine) {
  traitsPicked_.resize(0);
}

void PlayerBonHandler::getBgTitle(const Bg_t id, string& strRef) const {
  strRef = "[BG TITLE MISSING]";
  switch(id) {
    case bgOccultist: strRef = "Occultist"; break;
    case bgRogue:     strRef = "Rogue";     break;
    case bgSoldier:   strRef = "Soldier";   break;
    case endOfBgs: {} break;
  }
}

void PlayerBonHandler::getTraitTitle(
  const Trait_t id, string& strRef) const {

  strRef = "[TRAIT TITLE MISSING]";

  switch(id) {
    case traitAdeptMeleeCombatant:  strRef = "Adept Melee Combatant";   break;
    case traitExpertMeleeCombatant: strRef = "Expert Melee Combatant";  break;
    case traitMasterMeleeCombatant: strRef = "Master Melee Combatant";  break;
    case traitCoolHeaded:           strRef = "Cool-headed";             break;
    case traitCourageous:           strRef = "Courageous";              break;
    case traitSelfPossessed:        strRef = "Self-possessed";          break;
    case traitMythologist:          strRef = "Mythologist";             break;
    case traitDexterous:            strRef = "Dexterous";               break;
    case traitLithe:                strRef = "Lithe";                   break;
    case traitMobile:               strRef = "Mobile";                  break;
    case traitFearless:             strRef = "Fearless";                break;
    case traitHealer:               strRef = "Healer";                  break;
    case traitAdeptMarksman:        strRef = "Adept Marksman";          break;
    case traitExpertMarksman:       strRef = "Expert Marksman";         break;
    case traitMasterMarksman:       strRef = "Master Marksman";         break;
    case traitSteadyAimer:          strRef = "Steady Aimer";            break;
    case traitSharpShooter:         strRef = "Sharpshooter";            break;
    case traitObservant:            strRef = "Observant";               break;
    case traitVigilant:             strRef = "Vigilant";                break;
    case traitRapidRecoverer:       strRef = "Rapid Recoverer";         break;
    case traitSurvivalist:          strRef = "Survivalist";             break;
    case traitSelfAware:            strRef = "Self-aware";              break;
    case traitStrongSpirited:       strRef = "Strong-spirited";         break;
    case traitMightySpirited:       strRef = "Mighty-spirited";         break;
    case traitStealthy:             strRef = "Stealthy";                break;
    case traitImperceptible:        strRef = "Imperceptible";           break;
    case traitStrongBacked:         strRef = "Strong-backed";           break;
    case traitTough:                strRef = "Tough";                   break;
    case traitRugged:               strRef = "Rugged";                  break;
    case traitBreachExpert:         strRef = "Breach Expert";           break;
    case traitTreasureHunter:       strRef = "Treasure Hunter";         break;
    case traitDemolitionExpert:     strRef = "Demolition Expert";       break;
    case endOfTraits: break;
  }
}

void PlayerBonHandler::getBgDescr(const Bg_t id,
                                  vector<string>& linesRef) const {
  linesRef.resize(0);
  string s = "";

  switch(id) {
    case bgOccultist: {
      linesRef.push_back("Can memorize spells cast from manuscripts");
      linesRef.push_back("");
      linesRef.push_back("-2 Hit Point penalty");
      linesRef.push_back("");
      linesRef.push_back("Starts with the following trait(s):");
      linesRef.push_back("");
      getTraitTitle(traitStrongSpirited, s);      linesRef.push_back("* " + s);
      getTraitDescr(traitStrongSpirited, s);      linesRef.push_back(s);
    } break;

    case bgRogue: {
      linesRef.push_back("Can actively hide from monsters (press [h])");
      linesRef.push_back("");
      linesRef.push_back("Unaware monsters cause no shock");
      linesRef.push_back("");
      linesRef.push_back("Starts with the following trait(s):");
      linesRef.push_back("");
      getTraitTitle(traitObservant, s);           linesRef.push_back("* " + s);
      getTraitDescr(traitObservant, s);           linesRef.push_back(s);
      linesRef.push_back("");
      getTraitTitle(traitStealthy, s);            linesRef.push_back("* " + s);
      getTraitDescr(traitStealthy, s);            linesRef.push_back(s);
    } break;

    case bgSoldier: {
      linesRef.push_back("Starts with the following trait(s):");
      linesRef.push_back("");
      getTraitTitle(traitAdeptMarksman, s);       linesRef.push_back("* " + s);
      getTraitDescr(traitAdeptMarksman, s);       linesRef.push_back(s);
      linesRef.push_back("");
      getTraitTitle(traitAdeptMeleeCombatant, s); linesRef.push_back("* " + s);
      getTraitDescr(traitAdeptMeleeCombatant, s); linesRef.push_back(s);
      linesRef.push_back("");
      getTraitTitle(traitTough, s);               linesRef.push_back("* " + s);
      getTraitDescr(traitTough, s);               linesRef.push_back(s);
    } break;

    case endOfBgs: {} break;
  }
}

void PlayerBonHandler::getTraitDescr(
  const Trait_t id, string& strRef) const {

  strRef = "[TRAIT DESCRIPTION MISSING]";

  switch(id) {
    case traitAdeptMeleeCombatant: {
      strRef  = "+10% hit chance with melee weapons";
    } break;

    case traitExpertMeleeCombatant: {
      strRef  = "+10% hit chance with melee weapons";
    } break;

    case traitMasterMeleeCombatant: {
      strRef  = "+10% hit chance with melee weapons";
    } break;

    case traitAdeptMarksman: {
      strRef  = "+10% hit chance with firearms and thrown weapons";
    } break;

    case traitExpertMarksman: {
      strRef  = "+10% hit chance with firearms and thrown weapons, ";
      strRef += "you occasionally reload instantly";
    } break;

    case traitMasterMarksman: {
      strRef  = "+10% hit chance with firearms and thrown weapons";
    } break;

    case traitSteadyAimer: {
      strRef  = "Standing still gives ranged attacks +10% ";
      strRef += "hit chance on the following turn";
    } break;

    case traitSharpShooter: {
      strRef  = "Standing still for three turns gives ranged attacks ";
      strRef += "maximum hit chance and damage on the following turn";
    } break;

    case traitDemolitionExpert: {
      strRef = "Increased area of effect from Dynamite and Molotov Cocktails";
    } break;

    case traitCoolHeaded: {
      strRef  = "+20% shock resistance";
    } break;

    case traitCourageous: {
      strRef  = "+30% shock resistance";
    } break;

    case traitMythologist: {
      strRef  = "50% less shock taken from observing strange creatures, for ";
      strRef += "casting spells and using and identifying strange items ";
      strRef += "(e.g. potions)";
    } break;

    case traitSelfPossessed: {
      strRef  = "Passive shock received over time is reduced by 50% (does ";
      strRef += "not affect shock from seeing monsters, using magic, etc)";
    } break;

    case traitTough: {
      strRef  = "+2 hit points, +10% carry weight limit, better results ";
      strRef += "for object interactions requiring strength (e.g. bashing ";
      strRef += "doors or pushing a lid)";
    } break;

    case traitRugged: {
      strRef  = "+2 hit points, +10% carry weight limit, better results ";
      strRef += "for object interactions requiring strength (such as ";
      strRef += "bashing doors, or moving the lid from a stone coffin)";
    } break;

    case traitStrongBacked: {
      strRef  = "+30% carry weight limit";
    } break;

    case traitBreachExpert: {
      strRef  = "Increased chance of success when bashing doors, breaking ";
      strRef += "a door is a free turn, any creature adjacent to the door ";
      strRef += "on the opposite side is damaged and stunned";
    } break;

    case traitDexterous: {
      strRef  = "+25% chance to evade attacks and traps, ";
      strRef += "every fifth move is a free action";
    } break;

    case traitLithe: {
      strRef  = "+20% chance to evade attacks and traps, ";
      strRef += "every fourth move is a free action";
    } break;

    case traitMobile: {
      strRef  = "Every second move is a free action";
    } break;

    case traitFearless: {
      strRef  = "You cannot become terrified, +5% shock resistance";
    } break;

    case traitHealer: {
      strRef  = "Healing takes half the normal time and resources";
    } break;

    case traitObservant: {
      strRef  = "You occasionally spot clues about hidden passages, ";
      strRef += "your attentiveness is higher when examining objects ";
      strRef += "(e.g. when searching a chest for traps)";
    } break;

    case traitVigilant: {
      strRef  = "You cannot be backstabbed - hidden monsters gets no melee ";
      strRef += "attack bonus against you, and their attacks can be dodged";
    } break;

    case traitRapidRecoverer: {
      strRef  = "Increased Hit Point regeneration rate";
    } break;

    case traitSurvivalist: {
      strRef  = "Increased Hit Point regeneration rate, reduced penalties ";
      strRef += "from wounds and disease";
    } break;

    case traitSelfAware: {
      strRef  = "You cannot become confused, the number of remaining turns ";
      strRef += "for status effects are displayed";
    } break;

    case traitStrongSpirited: {
      strRef  = "+2 Spirit Points, increased Spirit Point regeneration rate";
    } break;

    case traitMightySpirited: {
      strRef  = "+2 Spirit Points, increased Spirit Point regeneration rate";
    } break;

    case traitStealthy: {
      strRef  = "You are more likely to avoid detection";
    } break;

    case traitImperceptible: {
      strRef  = "You are more likely to avoid detection";
    } break;

    case traitTreasureHunter: {
      strRef  = "You tend to find more items";
    } break;

    case endOfTraits: {} break;
  }
}

void PlayerBonHandler::getTraitPrereqs(const Trait_t id,
                                       vector<Trait_t>& traitsRef) const {
  traitsRef.resize(0);

  //TODO Add background prereqs
  //TODO Add CLVL prereqs

  switch(id) {
    case traitAdeptMeleeCombatant: {
    } break;

    case traitExpertMeleeCombatant: {
      traitsRef.push_back(traitAdeptMeleeCombatant);
    } break;

    case traitMasterMeleeCombatant: {
      traitsRef.push_back(traitExpertMeleeCombatant);
    } break;

    case traitAdeptMarksman: {
    } break;

    case traitExpertMarksman: {
      traitsRef.push_back(traitAdeptMarksman);
    } break;

    case traitMasterMarksman: {
      traitsRef.push_back(traitMasterMarksman);
    } break;

    case traitSteadyAimer: {
      traitsRef.push_back(traitAdeptMarksman);
    } break;

    case traitSharpShooter: {
      traitsRef.push_back(traitSteadyAimer);
      traitsRef.push_back(traitExpertMarksman);
    } break;

    case traitDemolitionExpert: {
    } break;

    case traitCoolHeaded: {
    } break;

    case traitCourageous: {
      traitsRef.push_back(traitCoolHeaded);
    } break;

    case traitMythologist: {
      traitsRef.push_back(traitCoolHeaded);
    } break;

    case traitSelfPossessed: {
      traitsRef.push_back(traitCoolHeaded);
    } break;

    case traitTough: {
    } break;

    case traitRugged: {
      traitsRef.push_back(traitTough);
    } break;

    case traitStrongBacked: {
      traitsRef.push_back(traitTough);
    } break;

    case traitBreachExpert: {
      traitsRef.push_back(traitTough);
    } break;

    case traitDexterous: {
    } break;

    case traitLithe: {
      traitsRef.push_back(traitDexterous);
    } break;

    case traitMobile: {
      traitsRef.push_back(traitLithe);
    } break;

    case traitFearless: {
      traitsRef.push_back(traitCoolHeaded);
    } break;

    case traitHealer: {
    } break;

    case traitObservant: {
    } break;

    case traitVigilant: {
      traitsRef.push_back(traitObservant);
    } break;

    case traitRapidRecoverer: {
      traitsRef.push_back(traitTough);
      traitsRef.push_back(traitHealer);
    } break;

    case traitSurvivalist: {
      traitsRef.push_back(traitRapidRecoverer);
    } break;

    case traitSelfAware: {
      traitsRef.push_back(traitStrongSpirited);
      traitsRef.push_back(traitObservant);
    } break;

    case traitStrongSpirited: {
    } break;

    case traitMightySpirited: {
      traitsRef.push_back(traitStrongSpirited);
    } break;

    case traitStealthy: {
    } break;

    case traitImperceptible: {
      traitsRef.push_back(traitStealthy);
      traitsRef.push_back(traitDexterous);
    } break;

    case traitTreasureHunter: {
      traitsRef.push_back(traitObservant);
    } break;

    case endOfTraits: {} break;
  }

  //Sort lexicographically
  sort(traitsRef.begin(), traitsRef.end(),
  [this](const Trait_t & t1, const Trait_t & t2) {
    string str1 = ""; getTraitTitle(t1, str1);
    string str2 = ""; getTraitTitle(t2, str2);
    return str1 < str2;
  });
}

void PlayerBonHandler::getAllPickableBgs(vector<Bg_t>& bgsRef) const {
  bgsRef.resize(0);

  for(int i = 0; i < endOfBgs; i++) {bgsRef.push_back(Bg_t(i));}

  //Sort lexicographically
  sort(bgsRef.begin(), bgsRef.end(),
  [this](const Bg_t & bg1, const Bg_t & bg2) {
    string str1 = ""; getBgTitle(bg1, str1);
    string str2 = ""; getBgTitle(bg2, str2);
    return str1 < str2;
  });
}

void PlayerBonHandler::getAllPickableTraits(vector<Trait_t>& traitsRef) const {
  traitsRef.resize(0);

  for(int i = 0; i < endOfTraits; i++) {

    const Trait_t trait = Trait_t(i);

    if(hasTrait(trait) == false) {

      vector<Trait_t> traitPrereqs;
      getTraitPrereqs(Trait_t(i), traitPrereqs);

      bool isPickable = true;
      for(Trait_t prereq : traitPrereqs) {

        if(hasTrait(prereq) == false) {
          isPickable = false;
          break;
        }
      }

      //TODO Check player background and CLVL prereqs

      if(isPickable) {
        traitsRef.push_back(Trait_t(i));
      }
    }
  }

  //Sort lexicographically
  sort(traitsRef.begin(), traitsRef.end(),
  [this](const Trait_t & t1, const Trait_t & t2) {
    string str1 = ""; getTraitTitle(t1, str1);
    string str2 = ""; getTraitTitle(t2, str2);
    return str1 < str2;
  });
}

void PlayerBonHandler::pickBg(const Bg_t bg) {
  assert(bg != endOfBgs);

  bg_ = bg;

  switch(bg_) {
    case bgOccultist: {
      pickTrait(traitStrongSpirited);
    } break;

    case bgRogue: {
      pickTrait(traitObservant);
      pickTrait(traitStealthy);
    } break;

    case bgSoldier: {
      pickTrait(traitAdeptMeleeCombatant);
      pickTrait(traitAdeptMarksman);
      pickTrait(traitTough);
    } break;

    case endOfBgs: {} break;
  }
}

void PlayerBonHandler::pickTrait(const Trait_t id) {
  assert(id != endOfTraits);

  traitsPicked_.push_back(id);

  switch(id) {
    case traitTough: {
      eng.player->changeMaxHp(2, false);
    } break;

    case traitRugged: {
      eng.player->changeMaxHp(2, false);
    } break;

    case traitStrongSpirited: {
      eng.player->changeMaxSpi(2, false);
    } break;

    case traitMightySpirited: {
      eng.player->changeMaxSpi(2, false);
    } break;

    case traitSelfAware: {
      eng.player->getPropHandler().tryApplyProp(
        new PropRConfusion(eng, propTurnsIndefinite), true, true, true, true);
    } break;

    case traitFearless: {
      eng.player->getPropHandler().tryApplyProp(
        new PropRFear(eng, propTurnsIndefinite), true, true, true, true);
    } break;

    //    case traitOccultist: {
    //      const int NR_SCROLLS_TO_START_WITH = 2;
    //      for(int i = 0; i < NR_SCROLLS_TO_START_WITH; i++) {
    //        Item* const item =
    //          eng.itemFactory->spawnRandomScrollOrPotion(true, false);
    //
    //        Spell_t spellId = item->getData().spellCastFromScroll;
    //        Spell* const spell = eng.spellHandler->getSpellFromId(spellId);
    //        const bool IS_SPELL_LEARNABLE = spell->isLearnableForPlayer();
    //        delete spell;
    //
    //        if(IS_SPELL_LEARNABLE && spellId != spell_pestilence) {
    //          Scroll* const scroll = dynamic_cast<Scroll*>(item);
    //          scroll->identify(true);
    //          eng.player->getInv().putItemInGeneral(scroll);
    //
    //          if(item->nrItems == 2) {
    //            item->nrItems = 1;
    //            i--;
    //          }
    //        } else {
    //          delete item;
    //          i--;
    //        }
    //      }
    //    } break;

    default: {} break;
  }
}

void PlayerBonHandler::getAllPickedTraitsTitlesLine(string& strRef) const {
  strRef = "";

  for(Trait_t t : traitsPicked_) {
    string title = ""; getTraitTitle(t, title);
    strRef += (strRef.empty() ? "" : ", ") + title;
  }
}
