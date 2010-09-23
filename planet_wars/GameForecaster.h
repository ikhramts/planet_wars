//Available under GPLv3.
//Author: Iouri Khramtsov.

#ifndef PLANET_WARRS_GAME_FORECASTER_H_
#define PLANET_WARRS_GAME_FORECASTER_H_

#include <vector>
#include "PlanetWars.h"
#include "Utils.h"

class PlanetForecaster;

typedef std::vector<PlanetForecaster*> PlanetForecasterList;

//This class is responsible for forecasting the state of the game.
class GameForecaster {
public:
    GameForecaster();
    ~GameForecaster();

    void SetGameMap(GameMap* game);

    //Recalculate the forecasts given the state of the game map.
    void Update();

    //Calculate how many additional ships will be gained by sending a set
    //of fleets to a planet.
    int ShipsGainedForFleets(const FleetList& fleets, Planet* planet) const;

    //Get the list of planets that will not be mine at any point in time
    //over the projected horizon.
    PlanetList PlanetsThatWillNotBeMine() const;

    //Find the number of ships needed to make sure that the specified planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(Planet* planet, int arrival_time) const;

    //Return the minimum number of ships required to stay on the planet to win any
    //upcoming battles.
    int ShipsRequiredToKeep(Planet* planet, int arrival_time) const;

private:
    int horizon_;
    GameMap* game_;
    PlanetForecasterList planet_forecasters_;
};

//A class for forecasting the state of each planet.
class PlanetForecaster {
public:
    PlanetForecaster();
    
    void Initialize(int forecast_horizon, Planet* planet, GameMap* game);
    
    void Update();

    //Calculate how many additional ships would be gained if specified
    //fleets would be sent to the planet.
    int ShipsGainedForFleets(const FleetList& fleets) const;

    bool WillNotBeMine() const              {return will_not_be_mine_;}
    
    Planet* GetPlanet() const               {return planet_;}

    //Find the number of ships needed to make sure that this planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(int arrival_time) const;

    //Return the minimum number of ships required to stay on the planet to win any
    //upcoming battles.
    int ShipsRequiredToKeep(int arrival_time) const;

private:
    //Update the planet state projections.
    void CalculatePlanetStateProjections(int starting_at);

    //Calculate actual index of an element in the *_at_ vectors
    //given a plain index.
    int ActualIndex(int i) const        {return (i + start_ + horizon_) % horizon_; }

    int horizon_;
    int start_;
    int end_;
    std::vector<int> owner_at_;
    std::vector<int> ships_at_;
    std::vector<int> my_arrivals_at_;
    std::vector<int> enemy_arrivals_at_;
    std::vector<int> ships_to_take_over_at_;
    std::vector<int> ships_gained_at_;
    std::vector<int> min_ships_to_keep_;        //Minimum ships required to keep the planet.

    int ships_gained_;

    //Indicates whether the planet will not be mine at any point
    //in the evaluated time frame.
    bool will_not_be_mine_;

    GameMap* game_;
    Planet* planet_;

    //Temporary storage.
    mutable std::vector<int> my_additional_arrivals_at_;
    mutable std::vector<int> enemy_additional_arrivals_at_;

};

#endif
