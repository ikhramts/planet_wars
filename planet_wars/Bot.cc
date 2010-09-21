//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#include "Bot.h"
#include "GameForecaster.h"

/************************************************
               GameForecaster class
************************************************/
Bot::Bot() 
: game_(NULL),
forecaster_(NULL) {
}

Bot::~Bot() {
    delete forecaster_;
}

Bot::SetGame(GameMap* game) {
    game_ = game;

    forecaster_ = new GameForecaster();
    forecaster_->SetGameMap(game);
}

FleetList Bot::MakeMoves() {
    forecaster_->Update();

    //General strategy: go for the combination of actions that provides
    //the highest %return of ships over a certain time horizon.
    const int time_horizon = 50;

    //Get the list of all planets
    PlanetList not_my_planets = game_map.NotMyPlanets();
    PlanetList my_planets = game_map.MyPlanets();
    const int num_not_my_planets = static_cast<int>(not_my_planets.size());
    const int num_my_planets = static_cast<int>(my_planets.size());
    
    //Compile the list of available ships.
    std::vector<int> remaining_ships_on_planets;

    for (int i = 0; i < num_my_planets; ++i) {
        remaining_ships_on_planets.push_back(my_planets[i].NumShips());
    }

    //Set up the list of invadeable planets.
    PlanetList invadeable_planets(not_my_planets);
    
    //The list of fleets to be ultimately sent.
    FleetList fleets_to_send;
    
    //Find out the base number of ships produced for each invadable planet.

    //Calculate the combinations of ships to send.
    while (true) {
        //Find the planet with the highest % return on investment
        //over the time horizon.
        std::vector<double> returns_for_planets;
        returns_for_planets.reserve(num_not_my_planets.size());
        std::vector<FleetList> invading_fleet_combinations;

        for (int p = 0; p < num_not_my_planets; ++p) {
            Planet target_planet = not_my_planets[p];
            const int planet_base_strength = target_planet.NumShips();
            const int planet_growth_rate = target_planet.GrowthRate();
            const bool is_enemy_owned = target_planet.IsEnemys();
            
            //Figure out the best combination of planets to use for attacking this
            //planet.
            PlanetList source_planets = game_map.NotMyPlanetsByDistance(target_planet);
            
            int turns_to_conquer = 0;
            int ships_to_send = 0;
            
            for (PlanetList::iterator it = source_planets.begin(); it != source_planets.end(); ++it) {
                Planet* source_planet = *it;

                turns_to_conquer = game_map.GetDistance(source_planet, target_planet);
                ships_to_send = target_planet.NumShipsIn(turns_to_conquer);


}