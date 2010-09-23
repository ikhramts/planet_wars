//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#include "Bot.h"
#include "GameTimeline.h"

/************************************************
               GameTimeline class
************************************************/
Bot::Bot() 
: game_(NULL),
timeline_(NULL) {
}

Bot::~Bot() {
    delete timeline_;
}

void Bot::SetGame(GameMap* game) {
    game_ = game;

    timeline_ = new GameTimeline();
    timeline_->SetGameMap(game);
}

ActionList Bot::MakeMoves() {
    if (game_->Turn() != 1) {
        timeline_->Update();

    } else {
        ActionPool* actionPool = new ActionPool();
    }

    //The list of fleets to be ultimately sent.
    ActionList final_actions;
    
    //Check if I've any ships left.
    if (0 == game_->NumShips(kMe)) {
        return final_actions;
    }

    //General strategy: go for the combination of actions that provides
    //the highest %return of ships over a certain time horizon.
    
    //Compile the list of available ships.
    PlanetList planets = game_->Planets();
    std::vector<int> available_ships_on_planets(planets.size());
    int num_ships_available = 0;
    
    for (uint i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        const int owner = planet->Owner();
        const int ships_on_planet = (kMe == owner ? planet->NumShips() : 0);
        const int ships_staying = (kMe == owner ? timeline_->ShipsRequiredToKeep(planet, 0) : 0);
        const int available_ships = std::max(ships_on_planet - ships_staying, 0);
        available_ships_on_planets[i] = available_ships;
        num_ships_available += available_ships;
    }

    if (0 == num_ships_available) {
        return final_actions;
    }

    //Set up the list of invadeable planets.
    PlanetTimelineList invadeable_planets = timeline_->PlanetsThatWillNotBeMine();
    
    while (invadeable_planets.size() != 0) {
        ActionList best_actions = this->BestRemainingMove(invadeable_planets);

        if (best_actions.empty()) {
            break;
        }

        //Add these actions to the list of other actions.
        for (uint i = 0; i < best_actions.size(); ++i) {
            final_actions.push_back(best_actions[i]);
        }
        
        timeline_->ApplyActions(best_actions);
    }

    return final_actions;

    ////////////////////////////////////////////////////////////////////////
    // END FUNCTION - the rest left over from other function.

    //Calculate the combinations of ships to send.
    while (invadeable_planets.size() != 0) {
        const uint num_invadeable_planets = invadeable_planets.size();

        //Find the planet with the highest % return on investment
        //over the time horizon.
        std::vector<double> returns_for_planets;
        returns_for_planets.reserve(num_invadeable_planets);
        std::vector<ActionList> actions_for_targets(num_invadeable_planets);

        for (uint p = 0; p < invadeable_planets.size(); ++p) {
            PlanetTimeline* target_planet = invadeable_planets[p];
            const int target_id = target_planet->Id();

            //Figure out the best combination of planets to use for attacking this
            //planet.
            PlanetList source_planets = game_->MyPlanetsByDistance(target_planet);
            ActionList& actions_from_sources = actions_for_targets[p];
            
            int turns_to_conquer = 0;
            int ships_to_send = 0;
            
            for (uint s = 0; s < source_planets.size(); ++s) {
                //Calculate how many ships are necessary to make a difference given how fast
                //the ships can get from this planet to the target.
                PlanetTimeline* source_planet = source_planets[s];
                const int source_id = source_planet->Id();

                //Never send ships back to the same planet.
                if (source_id == target_id) {
                    continue;
                }

                turns_to_conquer = game_->GetDistance(source_planet, target_planet);
                const int ships_needed = timeline_->ShipsRequredToPosess(target_planet, turns_to_conquer);
                
                //Check whether this planet would already get enough ships.
                //This is necessary because as turns_to_conquer increases, ships_needed may decrease.
                //TODO: adjust previously calculated ships to send for possible decrease
                //in ships needed.
                if (ships_needed <= ships_to_send) {
                    break;
                }
                
                //Add ships from this planet.
                const int available_ships = available_ships_on_planets[source_id];
                if(0 == available_ships) {
                    continue;
                }
                
                const int remaining_ships_needed = ships_needed - ships_to_send;
                const int ships_to_send_from_here = std::min(available_ships, remaining_ships_needed);
                
                Action* action = Action::Get();
                action->SetOwner(kMe);
                action->SetSource(source_planet);
                action->SetDestination(target_planet);
                action->SetDistance(turns_to_conquer);
                action->SetNumShips(ships_to_send_from_here);

                actions_from_sources.push_back(action);

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
                    timeline_->ShipsGainedForFleets(actions_from_sources, target_planet);
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
            for (uint target_id = 0; target_id < actions_for_targets.size(); ++target_id) {
                for (uint f = 0; f < actions_for_targets[target_id].size(); ++f) {
                    actions_for_targets[target_id][f]->Free();
                }
            }

            //There's no further thinking to do.
            //Quit the main outer loop.
            break;

        } else {
            //Add the target to the list of planets to invade.
            //Clean up the other fleets while we're at it.
            for (uint target_id = 0; target_id < actions_for_targets.size(); ++target_id) {
                if (target_id != best_target) {
                    for (uint f = 0; f < actions_for_targets[target_id].size(); ++f) {
                        actions_for_targets[target_id][f]->Free();
                    }
                
                } else {
                    for (uint f = 0; f < actions_for_targets[target_id].size(); ++f) {
                        Action* action = actions_for_targets[target_id][f];
                        final_actions.push_back(action);
                        available_ships_on_planets[action->Source()->Id()] -= action->NumShips();
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
    
    return final_actions;
}
