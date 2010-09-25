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
    int num_ships_available = 0;
    
    for (uint i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        const int owner = planet->Owner();
        const int ships_on_planet = (kMe == owner ? planet->NumShips() : 0);
        num_ships_available += ships_on_planet;
    }

    if (0 == num_ships_available) {
        return final_actions;
    }

    //Set up the list of invadeable planets.
    PlanetTimelineList invadeable_planets = timeline_->TimelinesThatWillNotBeMine();
    
    while (invadeable_planets.size() != 0) {
        ActionList best_actions = this->BestRemainingMove(invadeable_planets, kMe);

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
}

ActionList Bot::BestRemainingMove(PlanetTimelineList &invadeable_planets, const int player) {
	ActionList best_actions;
	//forceCrash();
	//Stop right here if there are no more ships to invade with.
	int current_free_ships = 0;
	PlanetTimelineList player_planets = timeline_->TimelinesOwnedBy(player);

	for (uint i = 0; i < player_planets.size(); ++i) {
		current_free_ships += player_planets[i]->ShipsFree(0, player);
	}

	if (0 == current_free_ships) {
		return best_actions;
	}
	
	//Proceed finding the best planet to invade.
	std::vector<std::vector<double> > plan_returns(invadeable_planets.size());
	std::vector<std::vector<ActionList> > plan_actions(invadeable_planets.size());

	const int horizon = timeline_->Horizon();
	const uint u_horizon = static_cast<uint>(horizon);
	
	for (uint i = 0; i < invadeable_planets.size(); ++i) {
		PlanetTimeline* target = invadeable_planets[i];
		const int target_id = target->Id();
		PlanetTimelineList sources = timeline_->EverOwnedTimelinesByDistance(player, target);
		std::vector<int> distances_to_sources(sources.size());

		for (uint s = 0; s < sources.size(); ++s) {
			distances_to_sources[s] = game_->GetDistance(sources[s]->Id(), target_id);
		}

		const uint first_source = sources[0]->Id() == target_id ? 1 : 0;

		//Don't proceed if the only source is the target itself.
		if (first_source == 1 && sources.size() == 1) {
			continue;
		}
		
		std::vector<double>& returns_for_target = plan_returns[i];
		returns_for_target.resize(u_horizon, 0);
		std::vector<ActionList>& actions_for_target = plan_actions[i];
		actions_for_target.resize(u_horizon);

		//Find earliest time the fleet can reach the target.
		const int earliest_arrival = distances_to_sources[first_source];

		for (int t = earliest_arrival; t < horizon; ++t) {
			//Find a possible invasion fleet, calculate the return on sending it.
			const int ships_needed = target->ShipsRequredToPosess(t, player);
			int ships_to_send = 0;

            for (uint s = first_source; s < sources.size(); ++s) {
                //Calculate how many ships are necessary to make a difference given how fast
                //the ships can get from this planet to the target.
                PlanetTimeline* source = sources[s];
                const int source_id = source->Id();

                //Never send ships back to the same planet.
                pw_assert(source_id != target_id);

				//Check whether the target is reacheable from the source.  If not, then it
				//won't be reacheable from any of the remaining sources.
                const int turns_to_conquer = distances_to_sources[s];
				
				if (turns_to_conquer > t) {
					break;
				}
				
                //Add ships from this planet.
                const int available_ships = source->ShipsFree(t - turns_to_conquer, player);
                if(0 == available_ships) {
                    continue;
                }
                
                const int remaining_ships_needed = ships_needed - ships_to_send;
                const int ships_to_send_from_here = std::min(available_ships, remaining_ships_needed);
                
                Action* action = Action::Get();
                action->SetOwner(player);
                action->SetSource(source);
                action->SetTarget(target);
                action->SetDistance(turns_to_conquer);
				action->SetDepartureTime(t - turns_to_conquer);
                action->SetNumShips(ships_to_send_from_here);

                actions_for_target[t].push_back(action);

                ships_to_send += ships_to_send_from_here;

				//Check whether this planet would already get enough ships.
                if (ships_needed <= ships_to_send) {
                    break;
                }
            } //End iterating over sources

            //Tally up the return on sending the fleets to this planet.
            if (0 == ships_to_send || ships_needed > ships_to_send) {
                continue;

            } else {
                const int ships_gained = 
					target->ShipsGainedForActions(actions_for_target[t]);
                const double return_ratio = static_cast<double>(ships_gained)
                                            / static_cast<double>(ships_to_send);

                returns_for_target[t] = return_ratio;
            }
		} //End iterating over arrival times.
	}//End iterating over targets.

	//Figure out which invasion plan is best.
	uint best_target = 0;
	int best_arrival_time = 0;
	double best_return = 0;

	for (uint i = 0; i < invadeable_planets.size(); ++i) {
		for (int t = horizon - 1; t >= 0; --t) {
			if (plan_returns[i][t] > best_return) {
				best_return = plan_returns[i][t];
				best_target = i;
				best_arrival_time = t;
			}
		}
	}

	//Save the best invasion plan.  Kill the rest.
	if (0 == best_return) {
		return best_actions;
	
	} else {
		for (uint i = 0; i < plan_actions.size(); ++i) {
			for (int t = 0; t < horizon; ++t) {
				ActionList& selected_actions = plan_actions[i][t];
				
				if (t == best_arrival_time && i == best_target) {
					for (uint a = 0; a < selected_actions.size(); ++a) {
						best_actions.push_back(selected_actions[a]);
					}
				
				} else {
					for (uint a = 0; a < selected_actions.size(); ++a) {
						selected_actions[a]->Free();
					}
				}
			}
		}

		//Remove the invaded planet from the list.
        const uint new_num_planets = invadeable_planets.size() - 1;
	    
		for (uint p = 0; p < new_num_planets; ++p) {
            if (p >= best_target) {
                invadeable_planets[p] = invadeable_planets[p+1];
            }
        }

        invadeable_planets.resize(new_num_planets);

	}
	
	//forceCrash();
	return best_actions;
}

