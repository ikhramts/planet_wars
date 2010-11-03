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
        return player_actions;
    }

    //Set up the list of invadeable planets.
    PlanetTimelineList invadeable_planets = timeline_->TimelinesEverNotOwnedBy(player);
    
    //forceCrash();
    const int earliest_departure = 0;
    std::vector<int> earliest_arrivals(invadeable_planets.size(), 0);
    std::vector<int> latest_arrivals(invadeable_planets.size(), timeline_->Horizon() - 8);
    const int depth = 1;
    
    picking_round_ = 1;

    while (invadeable_planets.size() != 0) {
        ActionList best_actions = this->BestRemainingMove(invadeable_planets, 
                                                          player, 
                                                          earliest_departure, 
                                                          earliest_arrivals,
                                                          latest_arrivals,
                                                          depth);

        if (best_actions.empty()) {
            break;
        
        } else {
            //Remove the planet being invaded.
            //const int target_id = best_actions[0]->Target()->Id();
            //bool found_target = false;
            //const uint planets_left = invadeable_planets.size() - 1;

            //for (uint i = 0; i < planets_left; ++i) {
            //    found_target |= (invadeable_planets[i]->Id() == target_id);

            //    if (found_target) {
            //        invadeable_planets[i] = invadeable_planets[i + 1];
            //    }
            //}

            //invadeable_planets.resize(planets_left);
            picking_round_++;
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

    //Pre-create frequently used vectors.
    PlanetTimelineList counter_action_targets;
    std::vector<int> earliest_counter_arrivals;
    std::vector<int> latest_counter_arrivals;
	
	//Proceed finding the best planet to invade.
    ActionList invasion_plan;
    invasion_plan.reserve(10);

	const int horizon = timeline_->Horizon();
	const uint u_horizon = static_cast<uint>(horizon);
    const int opponent = OtherPlayer(player);
	
    //Start searching for the best move.
	for (uint i = 0; i < invadeable_planets.size(); ++i) {
		PlanetTimeline* target = invadeable_planets[i];
		const int target_id = target->Id();

        if (target->GetPlanet()->GrowthRate() == 0) {
            continue;
        }

#ifndef IS_SUBMISSION
        if (depth == 1) {
            int x = 2;
        }
#endif

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

		//Don't proceed if the only source is the target itself.
		if (first_source == 1 && sources.size() == 1) {
			continue;
		}
		
        //Planets that might be helping defend the invasion, sorted by distance from target.
        PlanetTimelineList reinforcers = timeline_->EverOwnedTimelinesByDistance(opponent, target);
		std::vector<int> distances_to_reinforcers(reinforcers.size());

		for (uint s = 0; s < reinforcers.size(); ++s) {
			distances_to_reinforcers[s] = game_->GetDistance(reinforcers[s]->Id(), target_id);
		}

		const uint first_reinforcer = (reinforcers.empty() || reinforcers[0]->Id() != target_id) ? 0 : 1;

		//Find earliest time the fleet can reach the target.
        const int earliest_allowed_arrival = earliest_arrivals[i];
        const int earliest_possible_arrival = distances_to_sources[first_source] + earliest_allowed_departure;
        const int earliest_arrival = std::max(earliest_allowed_arrival, earliest_possible_arrival);
        
        int t = earliest_arrival;

        while (t < latest_arrivals[i]) {
			//Find a possible invasion fleet, calculate the return on sending it.
			int ships_needed = target->ShipsRequredToPosess(t, player);
            if (0 == ships_needed) {
                ++t;
                continue;
            }

			int ships_to_send = 0;
            int earliest_departure = t - distances_to_sources[first_source];
            uint next_reinforcer = first_reinforcer;

            double return_ratio = 0;

            const bool is_enemys = target->IsOwnedBy(opponent, t - 1);
            const bool is_neutral = target->IsOwnedBy(kNeutral, t - 1);
            const bool is_not_mine = !target->IsOwnedBy(player, t - 1);
            const bool was_enemys = (t > 1 && target->IsOwnedBy(opponent, t - 2));

            const double reinforcement_factor = (is_enemys && (player == kEnemy || was_enemys) ? 1.0 : 0.5);

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
                const int departure_time = t - turns_to_conquer;
                earliest_departure = departure_time;

				if (turns_to_conquer > (t - earliest_allowed_departure)) {
					break;
				}

                if (is_not_mine) {
                    //Add to the opponent's forces the reinforcing ships that the opponent
                    //can send by this time from other planets.
                    double d_reinforecements = 0;
                    while (next_reinforcer < reinforcers.size()) {
                        const int distance_to_reinforcer = distances_to_reinforcers[next_reinforcer];
                        
                        if (distance_to_reinforcer > turns_to_conquer) {
                            break;
                        
                        } else {
                            PlanetTimeline* reinforcer = reinforcers[next_reinforcer];
                            const int reinforcement_departure = t - distance_to_reinforcer;
                            const int reinforcements = reinforcer->ShipsFree(reinforcement_departure, opponent);
                            d_reinforecements += static_cast<double>(reinforcements * reinforcement_factor);
                            //ships_needed += reinforcements;
                            ++next_reinforcer;
                        }
                    }

                    ships_needed += static_cast<int>(d_reinforecements);
                }
				
                //Add ships from this planet.
                const int available_ships = source->ShipsFree(t - turns_to_conquer, player);
                if(0 == available_ships) {
                    continue;
                
                } else {
                    pw_assert(source->IsOwnedBy(player, t - turns_to_conquer));
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
                int ships_gained = 0;

                //Consider the opponent's possible responses
                if (depth > 0 && is_neutral) {

#ifndef IS_SUBMISSION
                    if (/*32 == turn_ && */20 == target_id && 8 == t && 3 == picking_round_) {
                        int adf = 2;
                    }
#endif
                    //Calculate what will happen if the opponent counterattacks the invasion target.
                    counter_action_targets.clear();
                    counter_action_targets.push_back(invasion_plan[0]->Target());

                    CounterActionResult target_counter_attack = 
                        this->ShipsGainedForAfterMove(invasion_plan, counter_action_targets, t);

                    //Calculate what will happen if the opponent counterattacks the invasion sources.
                    counter_action_targets.clear();
                    for (uint j = 0; j < invasion_plan.size(); ++j) {
                        counter_action_targets.push_back(invasion_plan[j]->Source());
                    }

                    CounterActionResult source_counter_attack = 
                        this->ShipsGainedForAfterMove(invasion_plan, counter_action_targets, t);

                    //Assume that the opponent will do the worst.
                    CounterActionResult* counter_attack;

                    if (target_counter_attack.ships_gained < source_counter_attack.ships_gained) {
                        counter_attack = &target_counter_attack;
                        Action::FreeActions(source_counter_attack.defense_plan);
                    
                    } else {
                        counter_attack = &source_counter_attack;
                        Action::FreeActions(target_counter_attack.defense_plan);
                    }
                    
                    ships_gained = counter_attack->ships_gained;

                    //Include the defense plan in the invasion plan.
                    for (uint k = 0; k < counter_attack->defense_plan.size(); ++k) {
                        Action* response = counter_attack->defense_plan[k];
                        response->SetContingent(true);
                        invasion_plan.push_back(response);
                    }

                    //Update the ships to commit to this.
                    ships_to_send = 0;
                    for (uint k = 0; k < invasion_plan.size(); ++k) {
                        ships_to_send += invasion_plan[k]->NumShips();
                    }

                } else {
                    ships_gained = target->ShipsGainedForActions(invasion_plan);
                }

                //const int ships_gained = timeline_->ShipsGainedFromBase();
                //const int ships_gained = target->ShipsGainedForActions(invasion_plan);
                return_ratio = static_cast<double>(ships_gained)
                                            / static_cast<double>(ships_to_send);
                

                //returns_for_target[t] = return_ratio;
                if (best_return < return_ratio) {
                    best_return = return_ratio;
                    best_actions = invasion_plan;
                    invasion_plan.clear();
                
                } else {
                    Action::FreeActions(invasion_plan);
                    invasion_plan.clear();
                }

            }

            //if (return_ratio > 0) {
            //    //Skip on to the next arrival or departure onto the planet.
            //    while (t < latest_arrivals[i]) {
            //        ++t;

            //        if (t < horizon && (target->MyArrivalsAt(t) > 0 || target->EnemyArrivalsAt(t) > 0)) {
            //            break;
            //        }
            //    }
            //
            //} else {
                ++t;
            //}
        } //End iterating over arrival times.
	}//End iterating over targets.

	return best_actions;
}

CounterActionResult Bot::ShipsGainedForAfterMove(const ActionList& invasion_plan, 
                                         PlanetTimelineList& counter_targets,
                                         const int attack_arrival_time) {
    timeline_->ApplyTempActions(invasion_plan);
    CounterActionResult result;
    const int horizon = timeline_->Horizon();

    //Find the best counter-attack involving the targets.
    const int player = invasion_plan[0]->Owner();
    const int opponent = OtherPlayer(player);
    PlanetTimeline* const target = invasion_plan[0]->Target();

    //Find the earliest and latest allowed departure/arrival times for the counter fleets.
    std::vector<int> earliest_arrivals;
    std::vector<int> latest_arrivals;

    for (uint i = 0; i < counter_targets.size(); ++i) {
        PlanetTimeline* const counter_target = counter_targets[i];

        if (counter_target == target) {
            if (target->IsOwnedBy(kNeutral, attack_arrival_time - 1)) {
                earliest_arrivals.push_back(attack_arrival_time);
                latest_arrivals.push_back(std::min(attack_arrival_time + counter_horizon_, horizon));

            } else if (target->IsOwnedBy(player, attack_arrival_time - 1)) {
                earliest_arrivals.push_back(attack_arrival_time - counter_horizon_);
                latest_arrivals.push_back(attack_arrival_time + 1);

            } else {
                earliest_arrivals.push_back(attack_arrival_time);
                latest_arrivals.push_back(attack_arrival_time + 1);
            }

        } else {
            //For sources of the original attack
            //Find the corresponding source of attack
            Action* action = NULL;

            for (uint j = 0; j < invasion_plan.size(); ++j) {
                action = invasion_plan[i];

                if (action->Source() == counter_target) {
                    break;
                }
            }

            earliest_arrivals.push_back(action->DepartureTime() + 1);
            latest_arrivals.push_back(horizon);
        }
    }

    //Find the earliest invasion plan departure.
    int earliest_departure = attack_arrival_time;
    for (uint i = 0; i < invasion_plan.size(); ++i) {
        earliest_departure = std::min(earliest_departure, invasion_plan[i]->DepartureTime() + 1);
    }


    ActionList best_counter_actions = this->BestRemainingMove(counter_targets, 
                                                                opponent, 
                                                                earliest_departure,
                                                                earliest_arrivals,
                                                                latest_arrivals, 
                                                                0 /* depth */);

    if (best_counter_actions.size() > 0) {
        timeline_->ApplyTempActions(best_counter_actions);

        //Find the best response to this counter-attack.
        PlanetTimelineList defense_targets;
        earliest_arrivals.clear();
        latest_arrivals.clear();
        
        PlanetTimeline* counter_target = best_counter_actions[0]->Target();
        defense_targets.push_back(counter_target);

        if (counter_target->IsOwnedBy(kNeutral, attack_arrival_time - 1)) {
            earliest_arrivals.push_back(attack_arrival_time);
            latest_arrivals.push_back(std::min(attack_arrival_time + counter_horizon_, horizon));

        } else if (counter_target->IsOwnedBy(player, attack_arrival_time - 1)) {
            earliest_arrivals.push_back(attack_arrival_time - counter_horizon_);
            latest_arrivals.push_back(attack_arrival_time + 1);

        } else {
            earliest_arrivals.push_back(attack_arrival_time);
            latest_arrivals.push_back(attack_arrival_time + 1);
        }

        int earliest_defense_departure = horizon;

        for (uint j = 0; j < best_counter_actions.size(); ++j) {
            Action* action = best_counter_actions[j];
            defense_targets.push_back(action->Source());
            earliest_arrivals.push_back(action->DepartureTime() + 1);
            latest_arrivals.push_back(horizon);

            const int possible_defense_departure = action->DepartureTime() + 1;

            if (earliest_defense_departure > possible_defense_departure) {
                earliest_defense_departure = possible_defense_departure;
            }
        }
        
        result.defense_plan = this->BestRemainingMove(defense_targets, 
                                    player, 
                                    earliest_defense_departure,
                                    earliest_arrivals,
                                    latest_arrivals, 
                                    0 /* depth */);

        ActionList& best_defense = result.defense_plan;

        timeline_->ApplyTempActions(best_defense);
    }

    result.ships_gained = timeline_->ShipsGainedFromBase();
    timeline_->ResetTimelinesToBase();
    
    //Update the ships to commit to this.
    Action::FreeActions(best_counter_actions);
    best_counter_actions.clear();
    
    return result;
}

ActionList Bot::SendFleetsToFront(const int player) {
    ActionList reinforcing_fleets;
    ActionList temp_action_list;
    temp_action_list.push_back(NULL);

    const int horizon = timeline_->Horizon();

    const int distance_threshold = 1;
    const int opponent = OtherPlayer(player);
    
    if (game_->PlanetsOwnedBy(opponent).empty()) {
        return reinforcing_fleets;
    }

    PlanetList player_planets = game_->PlanetsOwnedBy(player);
    PlanetTimelineList timelines = timeline_->Timelines();

    for (uint i = 0; i < player_planets.size(); ++i) {
        Planet* planet = player_planets[i];
        PlanetTimeline* planet_timeline = timelines[planet->Id()];
        const int free_ships = planet_timeline->ShipsFree(0, player);

        //Check whether the player owns any planets closer to enemy
        //planets than this one.
        PlanetList player_planets_by_distance = game_->PlayerPlanetsByDistance(player, planet);
        PlanetList targets_by_distance = game_->PlayerPlanetsByDistance(opponent, planet);
        //PlanetList targets_by_distance = game_->NotPlayerPlanetsByDistance(player, planet);

        pw_assert(!targets_by_distance.empty());

        //Find the closest non-zero growth planet not owned by the player.
        Planet* closest_target = NULL;
        for (uint j = 0; j < targets_by_distance.size(); ++j) {
            if (targets_by_distance[j]->GrowthRate() > 0) {
                closest_target = targets_by_distance[j];
                break;
            }
        }

        if (NULL == closest_target) {
            break;
        }

        const int distance_to_target = game_->GetDistance(planet, closest_target);

        //Find the best allied planet to reinforce.
        int send_ships_to = -1;
        int shortest_distance_to_target = distance_to_target;
        int distance_to_ally_planet = 0;

        for (uint p = 1; p < player_planets_by_distance.size(); ++p) {
            Planet* player_planet = player_planets_by_distance[p];
            const int planet_distance_to_target = game_->GetDistance(player_planet, closest_target);
            const int planet_distance_to_source = game_->GetDistance(player_planet, planet);

            if (planet_distance_to_target < shortest_distance_to_target
              && (distance_to_target - distance_threshold) > planet_distance_to_source) {
                shortest_distance_to_target = planet_distance_to_target;
                send_ships_to = player_planet->Id();
                distance_to_ally_planet = planet_distance_to_source;
            }
        }
        
        if (-1 != send_ships_to) {
            PlanetTimeline* forward_planet = timelines[send_ships_to];

            //Send all future available ships to the front line planet.
            for (int t = 0; t < horizon - distance_to_ally_planet; ++t) {
                const int turn_free_ships = planet_timeline->ShipsFree(t, player);

                if (0 != turn_free_ships) {
                    Action* action = Action::Get();
                    action->SetOwner(player);
                    action->SetSource(planet_timeline);
                    action->SetTarget(forward_planet);
                    action->SetDepartureTime(t);
                    action->SetDistance(distance_to_ally_planet);
                    action->SetNumShips(turn_free_ships);
                    temp_action_list[0] = action;

                    timeline_->ApplyTempActions(temp_action_list);
                    reinforcing_fleets.push_back(action);
                }
            }

            planet_timeline->SetReinforcer(true);
        }
    }
    
    timeline_->SaveTimelinesToBase();


    return reinforcing_fleets;
}

