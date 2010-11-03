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
timeline_(NULL),
turn_(0) {
}

Bot::~Bot() {
    delete timeline_;
}

void Bot::SetGame(GameMap* game) {
    game_ = game;

    timeline_ = new GameTimeline();
    timeline_->SetGameMap(game);

    counter_horizon_ = 20;
}

ActionList Bot::MakeMoves() {
    ++turn_;

    if (game_->Turn() != 1) {
        timeline_->Update();

    } else {
        ActionPool* actionPool = new ActionPool();
    }

    ActionList my_best_actions;
    
    //ActionList fleet_reinforcements = this->SendFleetsToFront(kMe);
    //my_best_actions.insert(my_best_actions.end(), fleet_reinforcements.begin(), fleet_reinforcements.end());

    ActionList found_actions = this->FindActionsFor(kMe); 
    my_best_actions.insert(my_best_actions.end(), found_actions.begin(), found_actions.end());

    return my_best_actions;
}

ActionList Bot::FindActionsFor(const int player) {
    //The list of fleets to be ultimately sent.
    ActionList player_actions;
    
    //Check if I've any ships left.
    if (0 == game_->NumShips(player)) {
        return player_actions;
    }

    //General strategy: go for the combination of actions that provides
    //the highest %return of ships over a certain time horizon.
    
    //forceCrash();
    picking_round_ = 1;

    while (true) {
        ActionList best_actions = this->BestRemainingMove(player);

        if (best_actions.empty()) {
            break;
        }        

        //Add these actions to the list of other actions.
        for (uint i = 0; i < best_actions.size(); ++i) {
            player_actions.push_back(best_actions[i]);
        }
        
#ifndef IS_SUBMISSION
        if (26 == turn_ && 3 == picking_round_) {
            int x = 2;
        }
#endif
        timeline_->ApplyActions(best_actions);
        picking_round_++;
    }

    return player_actions;
}

ActionList Bot::BestRemainingMove(const int player) {
    double best_return = 0;
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
    ActionList invasion_plan;
    invasion_plan.reserve(10);

	const int horizon = timeline_->Horizon();
	const uint u_horizon = static_cast<uint>(horizon);
    const int opponent = OtherPlayer(player);

    PlanetTimelineList targets = timeline_->Timelines();
	
    //Start searching for the best move.
	for (uint i = 0; i < targets.size(); ++i) {
		PlanetTimeline* target = targets[i];
		const int target_id = target->Id();
        const std::vector<int>& balances = target->Balances();

        if (target->GetPlanet()->GrowthRate() == 0) {
            continue;
        }

        //Planets that might be participating in the invasion, sorted by distance from target.
		PlanetTimelineList unfiltered_sources = timeline_->EverOwnedTimelinesByDistance(player, target);
        
        //Remove the feeder planets from the sources.
        PlanetTimelineList sources;
        
        for (uint j = 0; j < unfiltered_sources.size(); ++j) {
            if (!unfiltered_sources[j]->IsReinforcer()) {
                sources.push_back(unfiltered_sources[j]);
            }
        }
        
        //Pre-calculate distances to the sources.
		std::vector<int> distances_to_sources(sources.size());

		for (uint s = 0; s < sources.size(); ++s) {
			distances_to_sources[s] = game_->GetDistance(sources[s]->Id(), target_id);
		}

		const uint first_source = sources[0]->Id() == target_id ? 1 : 0;
        const uint distance_to_first_source = distances_to_sources[first_source];

		//Don't proceed if the only source is the target itself.
		if (first_source == 1 && sources.size() == 1) {
			continue;
		}
		
		//Try finding a good move for every arrival time.
        for (int t = distance_to_first_source; t < horizon; ++t) {
            const int owner = target->OwnerAt(t);
            const int balances_offset = t * (t - 1) / 2 - 1;
            const int ships_to_take_over = target->ShipsAt(t) + 1;

            int ships_needed = 0;

            if (player == owner && target->MinBalanceAt(t) < 0) {
                ships_needed = target->MinBalanceAt(t);
            
            } else if (player != owner && target->MaxBalanceAt(t) > 0) {
                ships_needed = std::max(balances[balances_offset + distance_to_first_source], ships_to_take_over);
            }

            if (0 == ships_needed) {
                continue;   //To the next arrival time t.
            }

            int ships_to_send = 0;
            bool was_invasion_plan_accepted = false;
            
            //Compose an invasion plan.
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
                
                if (player != owner) {
                    ships_needed = std::max(balances[balances_offset + turns_to_conquer], ships_to_take_over);
                }
                				
                //Add ships from this planet.
                const int available_ships = source->ShipsFree(t - turns_to_conquer, player);
                if(0 == available_ships) {
                    continue;
                
                } else {
                    pw_assert(source->IsOwnedBy(player, t - turns_to_conquer));
                }
                
                const int remaining_ships_needed = std::max(0, ships_needed - ships_to_send);
                
                if (remaining_ships_needed == 0) {
                    break;
                }

                const int ships_to_send_from_here = std::min(available_ships, remaining_ships_needed);
                
                Action* action = Action::Get();
                action->SetOwner(player);
                action->SetSource(source);
                action->SetTarget(target);
                action->SetDistance(turns_to_conquer);
				action->SetDepartureTime(t - turns_to_conquer);
                action->SetNumShips(ships_to_send_from_here);
                invasion_plan.push_back(action);

                ships_to_send += ships_to_send_from_here;

				//Check whether this planet would already get enough ships.
                if (ships_needed <= ships_to_send) {
                    break;
                }
            } //End iterating over sources

            //Tally up the return on sending the fleets to this planet.
            //Update the best invasion plan, if necessary.
            if (0 == ships_to_send || ships_needed > ships_to_send) {
                Action::FreeActions(invasion_plan);
                invasion_plan.clear();
                ++t;
                continue;

            } else {
#ifndef IS_SUBMISSION
                if (/*32 == turn_ && */20 == target_id && 8 == t && 3 == picking_round_) {
                    int adf = 2;
                }
#endif
                timeline_->ApplyTempActions(invasion_plan);

                //Check whether any of the source planets have acquired a more negative 
                //balance because of this invasion.
                PlanetTimelineList invasion_sources;
                for (uint j = 0; j < invasion_plan.size(); ++j) {
                    invasion_sources.push_back(invasion_plan[j]->Source());
                }

                if (!timeline_->HasNegativeBalanceWorsenedFor(invasion_sources)) {
                    //If all's good, compare the invasion plan to the others.
                    const int balance_gained = timeline_->NegativeBalanceImprovement();
                    const double return_ratio = 
                        static_cast<double>(balance_gained) / static_cast<double>(ships_to_send);

                    //returns_for_target[t] = return_ratio;
                    if (best_return < return_ratio) {
                        best_return = return_ratio;
                        Action::FreeActions(best_actions);
                        best_actions = invasion_plan;
                        invasion_plan.clear();
                    }
                }
            }

            Action::FreeActions(invasion_plan);
            invasion_plan.clear();
        } //End iterating over arrival times.
	}//End iterating over targets.

	return best_actions;
}

ActionList Bot::SendFleetsToFront(const int player) {
    ActionList reinforcing_fleets;

    const int horizon = timeline_->Horizon();
    
    PlanetTimelineList sources = timeline_->TimelinesOwnedBy(kMe, 0);
    PlanetTimelineList targets = timeline_->Timelines();

    for (uint i = 0; i < sources.size(); ++i) {
        PlanetTimeline* source = sources[i];

        //Figure out how many ships can be sent from this planet.
        int min_excess_balance = 100000;
        for (int t = 0; t < horizon; ++t) {
            if (min_excess_balance < source->MinBalanceAt(t)) {
                min_excess_balance = source->MinBalanceAt(t);
            }
        }

        const int available_ships = std::min(min_excess_balance, source->ShipsFree(0, kMe));

        if (available_ships <= 0) {
            continue;
        }

        Action* best_reinforecement = NULL;
        int best_balance_improvement = 0;

        //Figure out where it is best to send the ships.
        for (uint j = 0; j < targets.size(); ++j) {
            PlanetTimeline* target = targets[j];
            const int arrival_time = game_->GetDistance(source->Id(), target->Id());
            
            if (kMe == target->OwnerAt(arrival_time)) {
                Action* action = Action::Get();
                action->SetOwner(player);
                action->SetSource(source);
                action->SetTarget(target);
                action->SetDistance(arrival_time);
				action->SetDepartureTime(0);
                action->SetNumShips(available_ships);

                ActionList reinforecement_action_vector;
                reinforecement_action_vector.push_back(action);

                timeline_->ApplyTempActions(reinforecement_action_vector);
                const int balance_improvement = timeline_->NegativeBalanceImprovement();

                if (best_balance_improvement < balance_improvement) {
                    best_balance_improvement = balance_improvement;
                    best_reinforecement->Free();
                    best_reinforecement = action;
                
                } else {
                    Action::FreeActions(reinforecement_action_vector);
                }
            }
        }//End iterating over targets.

        if (NULL != best_reinforecement) {
            reinforcing_fleets.push_back(best_reinforecement);
        }
    }//End iterating over sources

    return reinforcing_fleets;
}

