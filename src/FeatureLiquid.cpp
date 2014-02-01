#include "FeatureLiquid.h"

#include "Engine.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Renderer.h"
#include "PlayerBonuses.h"
#include "Audio.h"

FeatureLiquidShallow::FeatureLiquidShallow(
  Feature_t id, Pos pos, Engine& engine) :
  FeatureStatic(id, pos, engine) {}

void FeatureLiquidShallow::bump(Actor& actorBumping) {
  const PropHandler& propHlr = actorBumping.getPropHandler();

  vector<PropId_t> props;
  actorBumping.getPropHandler().getAllActivePropIds(props);

  if(
    find(props.begin(), props.end(), propEthereal)  == props.end() &&
    find(props.begin(), props.end(), propFlying)    == props.end()) {

    actorBumping.getPropHandler().tryApplyProp(
      new PropWaiting(eng, propTurnsStd));

    //TODO For unknown reason, when player is walking in liquid, empty log
    //lines are created. This "glop" message masks the problem, but it
    //should be investigated and solved properly. Keep the "glop" message
    //though ;-)
    const bool IS_PLAYER = &actorBumping == eng.player;
    if(IS_PLAYER) {
      eng.log->addMsg("*glop*");
//      eng.audio->play(sfxGlop);
    }
  }
}

FeatureLiquidDeep::FeatureLiquidDeep(Feature_t id, Pos pos, Engine& engine) :
  FeatureStatic(id, pos, engine) {}

void FeatureLiquidDeep::bump(Actor& actorBumping) {
  (void)actorBumping;
}
