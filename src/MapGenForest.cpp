#include "MapGen.h"

#include <iostream>
#include <vector>

#include "Converters.h"
#include "ActorPlayer.h"
#include "ActorFactory.h"
#include "ActorMonster.h"
#include "Map.h"
#include "Highscore.h"
#include "Fov.h"
#include "TextFormatting.h"
#include "PopulateMonsters.h"
#include "MapParsing.h"
#include "Utils.h"
#include "FeatureStatic.h"

using namespace std;

namespace MapGen {

namespace IntroForest {

namespace {

void mkForestLimit() {
  auto putTree = [](const int X, const int Y) {Map::put(new Tree(Pos(X, Y)));};

  for(int y = 0; y < MAP_H; ++y) {putTree(0,          y);}
  for(int x = 0; x < MAP_W; ++x) {putTree(x,          0);}
  for(int y = 0; y < MAP_H; ++y) {putTree(MAP_W - 1,  y);}
  for(int x = 0; x < MAP_W; ++x) {putTree(x,          MAP_H - 1);}
}

void mkForestOuterTreeline() {
  const int MAX_LEN = 2;

  for(int y = 0; y < MAP_H; ++y) {
    for(int x = 0; x <= MAX_LEN; ++x) {
      if(Rnd::range(1, 4) > 1 || x == 0) {
        Map::put(new Tree(Pos(x, y)));
      } else {
        break;
      }
    }
  }

  for(int x = 0; x < MAP_W; ++x) {
    for(int y = 0; y < MAX_LEN; ++y) {
      if(Rnd::range(1, 4) > 1 || y == 0) {
        Map::put(new Tree(Pos(x, y)));
      } else {
        break;
      }
    }
  }

  for(int y = 0; y < MAP_H; ++y) {
    for(int x = MAP_W - 1; x >= MAP_W - MAX_LEN; x--) {
      if(Rnd::range(1, 4) > 1 || x == MAP_W - 1) {
        Map::put(new Tree(Pos(x, y)));
      } else {
        break;
      }
    }
  }

  for(int x = 0; x < MAP_W; ++x) {
    for(int y = MAP_H - 1; y >= MAP_H - MAX_LEN; y--) {
      if(Rnd::range(1, 4) > 1 || y == MAP_H - 1) {
        Map::put(new Tree(Pos(x, y)));
      } else {
        break;
      }
    }
  }
}

void mkForestTreePatch() {
  const int NR_TREES_TO_PUT = Rnd::range(5, 17);

  Pos curPos(Rnd::range(1, MAP_W - 2), Rnd::range(1, MAP_H - 2));

  int nrTreesCreated = 0;

  while(nrTreesCreated < NR_TREES_TO_PUT) {
    if(
      !Utils::isPosInsideMap(curPos) ||
      Utils::kingDist(curPos, Map::player->pos) <= 1) {
      return;
    }

    Map::put(new Tree(curPos));

    ++nrTreesCreated;

    //Find next pos
    while(
      Map::cells[curPos.x][curPos.y].featureStatic->getId() == FeatureId::tree ||
      Utils::kingDist(curPos, Map::player->pos) <= 2) {

      if(Rnd::coinToss()) {
        curPos.x += Rnd::coinToss() ? -1 : 1;
      } else {
        curPos.y += Rnd::coinToss() ? -1 : 1;
      }

      if(!Utils::isPosInsideMap(curPos)) {return;}
    }
  }
}

void mkForestTrees() {
  MapGenUtils::backupMap();

  const Pos churchPos(MAP_W - 33, 2);

  int nrForestPatches = Rnd::range(40, 55);

  vector<Pos> path;

  bool proceed = false;
  while(!proceed) {
    for(int i = 0; i < nrForestPatches; ++i) {
      mkForestTreePatch();
    }

    const MapTempl& templ     = MapTemplHandling::getTempl(MapTemplId::church);
    const Pos       templDims = templ.getDims();

    for(int y = 0; y < templDims.y; ++y) {
      for(int x = 0; x < templDims.x; ++x) {
        const auto id = templ.getCell(x, y).featureId;
        if(id != FeatureId::empty) {
          const Pos p(churchPos + Pos(x, y));
          Map::put(static_cast<FeatureStatic*>(FeatureData::getData(id).mkObj(p)));
        }
      }
    }

    Pos stairsPos;
    for(int y = 0; y < MAP_H; ++y) {
      bool isStairsFound = false;
      for(int x = 0; x < MAP_W; ++x) {
        if(Map::cells[x][y].featureStatic->getId() == FeatureId::stairs) {
          stairsPos.set(x, y);
          isStairsFound = true;
          break;
        }
      }
      if(isStairsFound) {break;}
    }

    bool blocked[MAP_W][MAP_H];
    MapParse::parse(CellPred::BlocksMoveCmn(false), blocked);

    blocked[stairsPos.x][stairsPos.y] = false;

    PathFind::run(Map::player->pos, stairsPos, blocked, path);

    size_t minPathLength = 1;
    size_t maxPathLength = 999;

    if(path.size() >= minPathLength && path.size() <= maxPathLength) {
      proceed = true;
    } else {
      MapGenUtils::restoreMap();
    }

    maxPathLength++;
  }

  //Build path
  for(const Pos& pathPos : path) {
    for(int dx = -1; dx < 1; ++dx) {
      for(int dy = -1; dy < 1; ++dy) {
        const Pos p(pathPos + Pos(dx, dy));
        if(
          Map::cells[p.x][p.y].featureStatic->canHaveStaticFeature() &&
          Utils::isPosInsideMap(p)) {
          Floor* const floor = new Floor(p);
          floor->type_ = FloorType::stonePath;
          Map::put(floor);
        }
      }
    }
  }

  //Place graves
  vector<HighScoreEntry> highscoreEntries = HighScore::getEntriesSorted();
  const int PLACE_TOP_N_HIGHSCORES = 7;
  const int NR_HIGHSCORES =
    min(PLACE_TOP_N_HIGHSCORES, int(highscoreEntries.size()));
  if(NR_HIGHSCORES > 0) {
    bool blocked[MAP_W][MAP_H];
    MapParse::parse(CellPred::BlocksMoveCmn(true), blocked);

    bool vision[MAP_W][MAP_H];

    const int SEARCH_RADI = FOV_STD_RADI_INT - 2;
    const int TRY_PLACE_EVERY_N_STEP = 2;

    vector<Pos> gravePositions;

    int pathWalkCount = 0;
    for(unsigned int i = 0; i < path.size(); ++i) {
      if(pathWalkCount == TRY_PLACE_EVERY_N_STEP) {

        Fov::runFovOnArray(blocked, path.at(i), vision, false);

        for(int dy = -SEARCH_RADI; dy <= SEARCH_RADI; ++dy) {
          for(int dx = -SEARCH_RADI; dx <= SEARCH_RADI; ++dx) {

            const int X = path.at(i).x + dx;
            const int Y = path.at(i).y + dy;

            const bool IS_LEFT_OF_CHURCH = X < churchPos.x - (SEARCH_RADI) + 2;
            const bool IS_ON_STONE_PATH =
              Map::cells[X][Y].featureStatic->getId() == FeatureId::floor;

            bool isLeftOfPrev = true;
            if(!gravePositions.empty()) {
              isLeftOfPrev = X < gravePositions.back().x;
            }

            bool isPosOk = vision[X][Y] && IS_LEFT_OF_CHURCH &&
                           !IS_ON_STONE_PATH && isLeftOfPrev;

            if(isPosOk) {
              for(int dy_small = -1; dy_small <= 1; dy_small++) {
                for(int dx_small = -1; dx_small <= 1; dx_small++) {
                  if(blocked[X + dx_small][Y + dy_small]) {
                    isPosOk = false;
                  }
                }
              }
              if(isPosOk) {
                gravePositions.push_back(Pos(X, Y));
                blocked[X][Y] = true;
                if(int(gravePositions.size()) == NR_HIGHSCORES) {i = 9999;}
                dy = 99999;
                dx = 99999;
              }
            }
          }
        }
        pathWalkCount = 0;
      }
      pathWalkCount++;
    }
    for(size_t i = 0; i < gravePositions.size(); ++i) {
      GraveStone* grave = new GraveStone(gravePositions[i]);
      HighScoreEntry curHighscore = highscoreEntries.at(i);
      const string name = curHighscore.getName();
      vector<string> dateStrVector;
      dateStrVector.resize(0);
      TextFormatting::getSpaceSeparatedList(curHighscore.getDateAndTime(),
                                            dateStrVector);
      const string date   = dateStrVector.at(0);
      const string score  = toStr(curHighscore.getScore());
      grave->setInscription("RIP " + name + " " + date + " Score: " + score);
      Map::put(grave);
    }
  }
}

} //namespace

bool run() {
  for(int y = 1; y < MAP_H - 1; ++y) {
    for(int x = 1; x < MAP_W - 1; ++x) {
      const Pos p(x, y);
      if(Rnd::oneIn(6)) {
        Map::put(new Bush(p));
      } else {
        Map::put(new Grass(p));
      }
    }
  }

  mkForestOuterTreeline();
  mkForestTrees();
  mkForestLimit();

  PopulateMonsters::populateIntroLvl();

  return true;
}

} //IntroForest

} //MapGen
