//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#ifndef PLANET_WARS_BOT_H_
#define PLANET_WARS_BOT_H_

#include "PlanetWars.h"
#include "GameTimeline.h"
#include "Actions.h"

class GameTimeline;

class Bot {
public:
    Bot();
    ~Bot();

    //Initialization.
    void SetGame(GameMap* game);

    //Decide which fleets to send.  This is the main move logic function.
    ActionList MakeMoves();

private:
    ActionList FindActionsFor(int player);
    ActionList BestRemainingMove(PlanetTimelineList& invadeable_planets, int player);

    GameMap* game_;
    GameTimeline* timeline_;
};


#endif
