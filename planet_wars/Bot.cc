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

void Bot::SetGame(GameMap* game) {
    game_ = game;

    forecaster_ = new GameForecaster();
    forecaster_->SetGameMap(game);
}

FleetList Bot::MakeMoves() {
    if (game_->Turn() != 1) {
        forecaster_->Update();
    }
    
    //The list of fleets to be ultimately sent.
    FleetList fleets_to_send;
    
    //Check if I've any ships left.
    if (0 == game_->NumShips(kMe)) {
        return fleets_to_send;
    }

    //General strategy: go for the combination of actions that provides
    //the highest %return of ships over a certain time horizon.
    
    //Compile the list of available ships.
    PlanetList planets = game_->Planets();
    std::vector<int> remaining_ships_on_planets(planets.size());
    int num_ships_available = 0;
    
    for (uint i = 0; i < planets.size(); ++i) {
        const int ships_available_here = (kMe == planets[i]->Owner() ? planets[i]->NumShips() : 0);
        remaining_ships_on_planets[i] = ships_available_here;
        num_ships_available += ships_available_here;
    }

    if (0 == num_ships_available) {
        return fleets_to_send;
    }

    //Set up the list of invadeable planets.
    PlanetList invadeable_planets = forecaster_->PlanetsThatWillNotBeMine();
    
    //Calculate the combinations of ships to send.
    while (invadeable_planets.size() != 0) {
        const uint num_invadeable_planets = invadeable_planets.size();

        //Find the planet with the highest % return on investment
        //over the time horizon.
        std::vector<double> returns_for_planets;
        returns_for_planets.reserve(num_invadeable_planets);
        std::vector<FleetList> fleets_for_targets(num_invadeable_planets);

        for (uint p = 0; p < invadeable_planets.size(); ++p) {
            Planet* target_planet = invadeable_planets[p];
            const int target_id = target_planet->Id();

            //Figure out the best combination of planets to use for attacking this
            //planet.
            PlanetList source_planets = game_->MyPlanetsByDistance(target_planet);
            FleetList& fleets_from_sources = fleets_for_targets[p];
            
            int turns_to_conquer = 0;
            int ships_to_send = 0;
            
            for (uint s = 0; s < source_planets.size(); ++s) {
                //Calculate how many ships are necessary to make a difference given how fast
                //the ships can get from this planet to the target.
                Planet* source_planet = source_planets[s];
                const int source_id = source_planet->Id();

                //Never send ships back to the same planet.
                if (source_id == target_id) {
                    continue;
                }

                turns_to_conquer = game_->GetDistance(source_planet, target_planet);
                const int ships_needed = forecaster_->ShipsRequredToPosess(target_planet, turns_to_conquer);
                
                //Check whether this planet would already get enough ships.
                //This is necessary because as turns_to_conquer increases, ships_needed may decrease.
                //TODO: adjust previously calculated ships to send for possible decrease
                //in ships needed.
                if (ships_needed <= ships_to_send) {
                    break;
                }
                
                //Add ships from this planet.
                const int available_ships = remaining_ships_on_planets[source_id];
                if(0 == available_ships) {
                    continue;
                }
                
                const int remaining_ships_needed = ships_needed - ships_to_send;
                const int ships_to_send_from_here = std::min(available_ships, remaining_ships_needed);
                
                Fleet* fleet = new Fleet();
                fleet->SetOwner(kMe);
                fleet->SetSource(source_planet);
                fleet->SetDestination(target_planet);
                fleet->SetTripLength(turns_to_conquer);
                fleet->SetNumShips(ships_to_send_from_here);

                fleets_from_sources.push_back(fleet);

                ships_to_send += ships_to_send_from_here;

                
                //Check whether we have now enough ships to make a difference.
                if (remaining_ships_needed <= available_ships) {
                    break;
                }
            }

            //Tally up the return on sending the fleets to this planet.
            
            if (0 == ships_to_send) {
                returns_for_planets.push_back(0);

            } else {
                const int ships_gained = 
                    forecaster_->ShipsGainedForFleets(fleets_from_sources, target_planet);
                const double return_ratio = static_cast<double>(ships_gained)
                                            / static_cast<double>(ships_to_send);

                returns_for_planets.push_back(return_ratio);
            }
        } // End iterating over targets.

        //Find the planet with the largest return.
        int best_target = 0;
        double best_return = 0;

        for (uint p = 0; p < returns_for_planets.size(); ++p) {
            if (returns_for_planets[p] > best_return) {
                best_target = p;
                best_return = returns_for_planets[p];
            }
        }

        if (0 == best_return) {
            //Clean up all fleets.
            for (uint target_id = 0; target_id < fleets_for_targets.size(); ++target_id) {
                for (uint f = 0; f < fleets_for_targets[target_id].size(); ++f) {
                    delete fleets_for_targets[target_id][f];
                }
            }

            //There's no further thinking to do.
            //Quit the main outer loop.
            break;

        } else {
            //Add the target to the list of planets to invade.
            //Clean up the other fleets while we're at it.
            for (uint target_id = 0; target_id < fleets_for_targets.size(); ++target_id) {
                if (target_id != best_target) {
                    for (uint f = 0; f < fleets_for_targets[target_id].size(); ++f) {
                        delete fleets_for_targets[target_id][f];
                    }
                
                } else {
                    for (uint f = 0; f < fleets_for_targets[target_id].size(); ++f) {
                        Fleet* fleet = fleets_for_targets[target_id][f];
                        fleets_to_send.push_back(fleet);
                        remaining_ships_on_planets[fleet->Source()->Id()] -= fleet->NumShips();
                    }
                }
            }

            //Update the number of invadeable planets.
            const uint new_num_planets = invadeable_planets.size() - 1;
            const uint u_best_target = static_cast<uint>(best_target);

            for (uint p = 0; p < new_num_planets; ++p) {
                if (p >= u_best_target) {
                    invadeable_planets[p] = invadeable_planets[p+1];
                }
            }

            invadeable_planets.resize(new_num_planets);
        } //if (0 == best_return)
        
    } //End iteration over best invadeable planets.
    
    return fleets_to_send;
}