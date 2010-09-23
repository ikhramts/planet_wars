//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#ifndef PLANET_WARS_BOT_H_
#define PLANET_WARS_BOT_H_

#include "PlanetWars.h"

class GameForecaster;

class Bot {
public:
    Bot();
    ~Bot();

    //Initialization.
    void SetGame(GameMap* game);

    //Decide which fleets to send.  This is the main move logic function.
    FleetList MakeMoves();

private:
    GameMap* game_;
    GameForecaster* forecaster_;
};


#endif
