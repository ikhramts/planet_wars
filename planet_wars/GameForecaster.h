//Available under GPLv3.
//Author: Iouri Khramtsov.

#ifndef PLANET_WARRS_GAME_FORECASTER_H_
#define PLANET_WARRS_GAME_FORECASTER_H_

#include <vector>
#include "PlanetWars.h"

class PlanetForecaster;

typedef std::vector<PlanetForecaster*> PlanetForecasterList;

//This class is responsible for forecasting the state of the game.
class GameForecaster {
public:
    static const int kTimeHorizon = 50;

    GameForecaster();

    void SetGameMap(GameMap* game);

    //Recalculate the forecasts given the state of the game map.
    void Update();

    //Get the list of planets that will be not mine for at least
    //one turn over the specified time horizon.
    void PlanetsNotMineOverHorizon()        {return planets_not_mine_at_least_once_;}
    
    //Get the number of ships I will gain from the planet over the time horizon.
    //Negative numbers indicate enemy gaining ships.
    int ShipsGainedFrom(Planet* planet) const;

private:
    //Update the forecast data for a single planet.
    void UpdatePlanetForecast(Planet* planet);

    PlanetList planets_not_mine_at_least_once_;
    std::vector<int> ships_gained_from_planets_;
    GameMap* game_;
    PlanetForecasterList planet_forecasters_;
};

//A class for forecasting the state of each planet.
class PlanetForecaster {
public:
    PlanetForecaster();
    
    void Initialize(int forecast_horizon, Planet* planet, GameMap* game);
    
    void Update();

private:
    //Update the planet state projections.
    void CalculatePlanetStateProjections(int starting_at);

    int horizon_;
    int start_;
    int end_;
    std::vector<int> owner_at_;
    std::vector<int> ships_at_;
    std::vector<int> my_arrivals_at_;
    std::vector<int> enemy_arrivals_at_;
    std::vector<int> ships_to_take_over_at_;

    GameMap* game_;
    Planet* planet_;

};

#endif