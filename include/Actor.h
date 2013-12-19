#ifndef ACTOR_H
#define ACTOR_H

#include <string>
#include <vector>

#include "CommonData.h"

#include "ActorData.h"
#include "Sound.h"
#include "Config.h"
#include "Art.h"

using namespace std;

class Engine;

class PropHandler;
class Inventory;

class Actor {
public:
  Actor(Engine& engine);
  virtual ~Actor();

  inline PropHandler* getPropHandler() {return propHandler_;}

  inline ActorData* getData() {return data_;}

  Inventory* getInventory() {return inventory_;}

  void place(const Pos& pos_, ActorData& data);

  bool hit(int dmg, const DmgTypes_t dmgType, const bool ALLOW_WOUNDS);
  bool hitSpi(const int DMG);

  bool restoreHp(const int HP_RESTORED,
                 const bool ALLOW_MESSAGES = true,
                 const bool IS_ALLOWED_ABOVE_MAX = false);
  bool restoreSpi(const int SPI_RESTORED,
                  const bool ALLOW_MESSAGES = true,
                  const bool IS_ALLOWED_ABOVE_MAX = false);
  void changeMaxHp(const int CHANGE, const bool ALLOW_MESSAGES);
  void changeMaxSpi(const int CHANGE, const bool ALLOW_MESSAGES);

  void die(const bool IS_MANGLED, const bool ALLOW_GORE,
           const bool ALLOW_DROP_ITEMS);

  virtual void onActorTurn() {}
  virtual void actorSpecificOnStandardTurn() {}

  virtual void moveDir(Dir_t dir) = 0;

  virtual void updateColor();

  //Function taking into account FOV, invisibility, status, etc
  //This is the final word on wether an actor can visually percieve
  //another actor.
  bool checkIfSeeActor(
    const Actor& other,
    const bool visionBlockingCells[MAP_X_CELLS][MAP_Y_CELLS]) const;

  void getSpotedEnemies(vector<Actor*>& vectorRef);

  inline ActorId_t getId()  const {return data_->id;}
  inline int getHp()        const {return hp_;}
  inline int getSpi()       const {return spi_;}
  virtual int getHpMax(const bool WITH_MODIFIERS) const {
    (void)WITH_MODIFIERS;
    return hpMax_;
  }
  inline int getSpiMax()    const {return spiMax_;}

  inline string getNameThe() const {return data_->name_the;}
  inline string getNameA() const {return data_->name_a;}
  inline bool isHumanoid() const {return data_->isHumanoid;}
  inline char getGlyph() const {return glyph_;}
  virtual const SDL_Color& getColor() {return clr_;}
  inline const Tile_t& getTile() const {return tile_;}
  inline BodyType_t getBodyType() const {return data_->bodyType;}

  void addLight(bool light[MAP_X_CELLS][MAP_Y_CELLS]) const;

  virtual void actorSpecific_addLight(
    bool light[MAP_X_CELLS][MAP_Y_CELLS]) const {
    (void)light;
  }

  void teleport(const bool MOVE_TO_POS_AWAY_FROM_MONSTERS);

  Pos pos;
  ActorDeadState_t deadState;

  Engine& eng;

protected:
  //TODO Try to get rid of these friend declarations
  friend class AbilityValues;
  friend class DungeonMaster;
  friend class Dynamite;
  friend class Molotov;
  friend class Flare;
  friend class PropDiseased;

  virtual void actorSpecificDie() {}
  virtual void actorSpecific_hit(const int DMG, const bool ALLOW_WOUNDS) {
    (void)DMG;
    (void)ALLOW_WOUNDS;
  }
  virtual void actorSpecific_spawnStartItems() = 0;

  virtual void onMonsterHit(int& dmg) {(void)dmg;}
  virtual void onMonsterDeath() {}

  SDL_Color clr_;
  char glyph_;
  Tile_t tile_;

  int hp_, hpMax_, spi_, spiMax_;

  Pos lairCell_;

  PropHandler*  propHandler_;
  ActorData*    data_;
  Inventory*    inventory_;
};

#endif
