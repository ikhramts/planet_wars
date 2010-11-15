//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#include "Bot.h"
#include "GameTimeline.h"
#include "Timer.h"

#ifndef IS_SUBMISSION
#include <iostream>
#include <sstream>
#endif

/************************************************
               GameTimeline class
************************************************/
const double Bot::kAggressionReturnMultiplier = 3;

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
    defense_horizon_ = 4;

    const int num_planets = game->NumPlanets();
    when_is_feeder_allowed_to_attack_.resize(num_planets * num_planets, -1);

    support_constraints_ = new SupportConstraints(game_->NumPlanets(), game_);
}

ActionList Bot::MakeMoves() {
    ++turn_;

    if (game_->Turn() != 1) {
        timeline_->Update();

    } else {
        ActionPool* actionPool = new ActionPool();
    }

#ifndef IS_SUBMISSION
    timeline_->AssertWorkingTimelinesAreEqualToBase();
#endif

    ActionList my_best_actions;
    
    //Update feeder planet attack permissions.
    for (uint i = 0; i < when_is_feeder_allowed_to_attack_.size(); ++i) {
        if (when_is_feeder_allowed_to_attack_[i] > -1) {
            --when_is_feeder_allowed_to_attack_[i];
        }
    }

    support_constraints_->ClearConstraints();

    timeline_->SetFeederAttackPermissions(&when_is_feeder_allowed_to_attack_);

#ifdef MAKE_ENEMY_MOVES_ON_FIRST_TURN
    if (1 == turn_) {
        //On the first turn, consider what the enemy would do.
        ActionList enemy_actions = this->FindActionsFor(kEnemy);
    }
#endif

    //Mark the reinforcers.
    this->MarkReinforcers(kMe);

    ActionList found_actions = this->FindActionsFor(kMe); 
    my_best_actions.insert(my_best_actions.end(), found_actions.begin(), found_actions.end());
    
    ActionList fleet_reinforcements = this->SendFleetsToFront(kMe);
    my_best_actions.insert(my_best_actions.end(), fleet_reinforcements.begin(), fleet_reinforcements.end());

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
    //the highest % return of ships over a certain time horizon.
    
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
    PlanetTimelineList invadeable_planets = timeline_->Timelines();
    
    //forceCrash();
    const int earliest_departure = 0;
    std::vector<int> earliest_arrivals(invadeable_planets.size(), 0);
    std::vector<int> latest_arrivals(invadeable_planets.size(), timeline_->Horizon() - 8);
    const int depth = 0;
    
    picking_round_ = 1;

    while (invadeable_planets.size() != 0) {
#ifdef PRE_APPLY_SUPPORT_ACTIONS
        ActionList support_actions = this->SendSupportFleets(kMe);
        player_actions.insert(player_actions.end(), support_actions.begin(), support_actions.end());
#endif

        ActionList best_actions = this->BestRemainingMove(invadeable_planets, 
                                                          player, 
                                                          earliest_departure, 
                                                          earliest_arrivals,
                                                          latest_arrivals,
                                                          depth);

        if (best_actions.empty()) {
            break;
        }
        
        picking_round_++;

        //Add these actions to the list of other actions.
        for (uint i = 0; i < best_actions.size(); ++i) {
            player_actions.push_back(best_actions[i]);
        }

        //Check whether we need to add permissions for possible future feeder planets
        //to attack an enemy planet.
        if (best_actions[best_actions.size() - 1]->DepartureTime() == 0) {
            const int num_planets = game_->NumPlanets();

            for (uint i = 0; i < best_actions.size(); ++i) {
                Action* action = best_actions[i];
                const int source_id = action->Source()->Id();
                const int target_id = action->Target()->Id();
                when_is_feeder_allowed_to_attack_[source_id * num_planets + target_id] = action->DepartureTime();
            }
        }

        this->ApplyActions(best_actions);
        //timeline_->ApplyActions(best_actions);
    }

    return player_actions;
}

ActionList Bot::BestRemainingMove(PlanetTimelineList& invadeable_planets, 
                                 const int player,
                                 const int earliest_allowed_departure,
                                 const std::vector<int>& earliest_arrivals,
                                 const std::vector<int>& latest_arrivals,
                                 const int depth) {
    double best_return = 0;
	ActionList best_actions;
    bool has_timed_out = false;
    
	//Stop right here if there are no more ships to invade with.
	int current_free_ships = 0;
	PlanetTimelineList player_planets = timeline_->TimelinesOwnedBy(player);

	for (uint i = 0; i < player_planets.size(); ++i) {
		current_free_ships += player_planets[i]->ShipsFree(0, player);
	}

	if (0 == current_free_ships) {
		return best_actions;
	}

	const int horizon = timeline_->Horizon();
	const uint u_horizon = static_cast<uint>(horizon);
    const int opponent = OtherPlayer(player);
    const int num_planets = game_->NumPlanets();

    //Pre-create frequently used vectors.
    PlanetTimelineList counter_action_targets;
    std::vector<int> earliest_counter_arrivals;
    std::vector<int> latest_counter_arrivals;
	
	//Proceed finding the best planet to invade.
    ActionList invasion_plan;
    invasion_plan.reserve(10);
	
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
		PlanetTimelineList sources = timeline_->EverOwnedTimelinesByDistance(player, target);

        if (sources.empty()) {
            continue;   //To the next arrival time.
        }
        
        //Pre-calculate distances to the sources.
		std::vector<int> distances_to_sources(sources.size());

		for (uint s = 0; s < sources.size(); ++s) {
			distances_to_sources[s] = game_->GetDistance(sources[s]->Id(), target_id);
		}

		//Find earliest time the fleet can reach the target.
        const int earliest_allowed_arrival = earliest_arrivals[i];
        const int earliest_possible_arrival = distances_to_sources[0] + earliest_allowed_departure;
        const int earliest_arrival = std::max(earliest_allowed_arrival, earliest_possible_arrival);
        
#ifndef IS_SUBMISSION
        if (1 == picking_round_ && 18 == target_id) {
            int x = 2;
        }
#endif

        for (int arrival_time = earliest_arrival; arrival_time < latest_arrivals[i]; ++arrival_time) {
#ifndef IS_SUBMISSION
            if (1 == picking_round_ && 11 == target_id && 7 == arrival_time) {
                int x = 2;
            }
#endif

#ifdef USE_POTENTIAL_FUTURE_GAINS
            const int potential_ships_gained = target->PotentialGainsAfter(arrival_time);
            const int base_ships_gained = target->ShipsGained();
            bool was_permitted = true;
            if (potential_ships_gained - base_ships_gained <= 0) {
                //continue;
                was_permitted = false;
            }
#endif

            invasion_plan = this->FindInvasionPlan(target, arrival_time, sources, distances_to_sources, player);            

            //Check whether this move is better than any other we've seen so far.
            if (!invasion_plan.empty()) {
                const double return_ratio = this->ReturnForMove(invasion_plan, best_return);
                //const double return_ratio = this->ReturnForMove2(invasion_plan, best_return, depth);

                if (best_return < return_ratio) {
                    pw_assert(was_permitted && "Didn't permit a good move to pass");
                    best_return = return_ratio;
                    Action::FreeActions(best_actions);
                    best_actions = invasion_plan;
                    invasion_plan.clear();

                } else {
                    Action::FreeActions(invasion_plan);
                    invasion_plan.clear();
                }
            }

#ifdef WITH_TIMEOUTS
            if (HasTimedOut()) {
                has_timed_out = true;
                break;
            }
#endif

        }

        if (has_timed_out) {
            break;
        }
	}//End iterating over targets.

    if (has_timed_out) {
        //Don't send incomplete answers.
        Action::FreeActions(best_actions);
        best_actions.clear();
    }

	return best_actions;
}

ActionList Bot::FindInvasionPlan(PlanetTimeline* target, 
                                 const int arrival_time, 
                                 const PlanetTimelineList& sources, 
                                 const std::vector<int>& distances_to_sources,
                                 const int player) {
    ActionList invasion_plan;

    //Various useful variables.
    const int target_id = target->Id();
    const int target_owner = target->OwnerAt(arrival_time);
    const int potentials_offset = arrival_time * (arrival_time - 1) / 2 - 1;
    const int horizon = timeline_->Horizon();
    const int u_horizon = static_cast<uint>(horizon);
    const int num_planets = game_->NumPlanets();
    const std::vector<int>& defense_potentials = target->DefensePotentials();
    const int opponent = OtherPlayer(player);
    const int distance_to_first_source = distances_to_sources[0];
    std::vector<int> ships_farther_than(u_horizon, 0);

    //Possible adjustment for neutral planets in case of an opponent.
    const int neutral_adjustment = 
        ((player == kEnemy && target_owner == kNeutral) ? (2 * target->ShipsAt(arrival_time - 1)) : 0);

	//Minimum number of ships to conquer the planet.  More may be requred to 
    //fight off any other ships the opponent might send.
    int remaining_ships_to_take_over = target->ShipsRequredToPosess(arrival_time, player);
	
    //Figure out how many ships might be actually needed.  These values would be adjusted
    //duri the calculation.
    const int min_defense_potential = target->MinDefensePotentialAt(arrival_time);
    int remaining_ships_needed = 0;
    int max_ships_from_this_distance = 0;
    const int was_my_planet = (target->OwnerAt(arrival_time - 1) == player);
    const int takeover_ship = (was_my_planet ? 0 : 1);

    if ((player == target_owner || was_my_planet) && min_defense_potential < 0) {
        remaining_ships_needed = -min_defense_potential;

        //Find the minimum distances from which some of the ships need to be sent.
        int ships_farther_than_this_distance = 0;
        for (int d = arrival_time; d >= distance_to_first_source; --d) {
            const int ships_from_this_distance = 
                std::max(0, -defense_potentials[potentials_offset + d] - ships_farther_than_this_distance);
            ships_farther_than[d] = ships_from_this_distance;
            ships_farther_than_this_distance += ships_from_this_distance;
        }

        ships_farther_than[distance_to_first_source] += (remaining_ships_needed - ships_farther_than_this_distance);
        max_ships_from_this_distance = ships_farther_than[distance_to_first_source];

    } else if (player == target_owner && 0 == min_defense_potential && kEnemy == target->OwnerAt(arrival_time -1 )) {
        remaining_ships_needed = 1;
        max_ships_from_this_distance = 1;

#ifdef USE_SUPPORT_POTENTIALS_FOR_ATTACK
    } else if (player != target_owner && target->MaxSupportPotentialAt(arrival_time) > 0) {
#else
    } else if (player != target_owner && target->MaxDefensePotentialAt(arrival_time + 1) > 0) {
#endif
        const int ships_from_balance = 
            -defense_potentials[potentials_offset + distance_to_first_source] + neutral_adjustment + takeover_ship;
        remaining_ships_needed = std::max(ships_from_balance, remaining_ships_to_take_over);
        max_ships_from_this_distance = remaining_ships_needed;
    }

    if (0 == remaining_ships_needed) {
        return invasion_plan;   //Nothing to do.
    }

    //Compose the invasion plan.
	int ships_to_send = 0;
    int current_distance = distances_to_sources[0];

    for (uint s = 0; s < sources.size(); ++s) {
        pw_assert(remaining_ships_needed > 0);
        PlanetTimeline* source = sources[s];
        const int source_id = source->Id();

		//Check whether the target is reacheable from the source.  If not, then it
		//won't be reacheable from any of the remaining sources.
        const int distance_to_source = distances_to_sources[s];
        const int departure_time = arrival_time - distance_to_source;

		//if (distance_to_source > (arrival_time - earliest_allowed_departure)) {
		if (distance_to_source > (arrival_time)) {
            break;
		}

        //Feeder planets may only attack neutrals or support player's planets.
        if (source->IsReinforcer() && opponent == target_owner && !was_my_planet) {
            if (when_is_feeder_allowed_to_attack_[source_id * num_planets + target_id] != departure_time) {
                continue;   //To the next source.
            }
        }

        //Never send ships back to the same planet.
        pw_assert(source_id != target_id);

        //Adjust the number of ships needed to take out or support this planet for
        //any possible reinforcing enemies.
        if (distance_to_source > current_distance) {
            //if (target_owner == kEnemy) {
            if (target_owner != player && !was_my_planet) {
                const int ships_from_balance = 
                    -defense_potentials[potentials_offset + distance_to_source] + neutral_adjustment + takeover_ship;
                remaining_ships_needed = std::max(ships_from_balance, remaining_ships_to_take_over);
                max_ships_from_this_distance = remaining_ships_needed;
            
            } else if(target_owner == player || was_my_planet) {
                for (int d = current_distance + 1; d <= distance_to_source; ++d) {
                    max_ships_from_this_distance += ships_farther_than[d];
                }
            }

            current_distance = distance_to_source;
        }

        //Check whether the source is allowed to send ships to the target.
#ifdef USE_SUPPORT_CONSTRAINTS
        if (target_owner == player && !support_constraints_->MaySupport(source, target)) {
            continue;
        }
#endif
        
        //Check whether the source has any ships to send.
        const int available_ships = source->ShipsFree(arrival_time - distance_to_source, player);
        if(0 == available_ships) {
            continue;
        
        } else {
            pw_assert(source->IsOwnedBy(player, arrival_time - distance_to_source));
        }

        if (max_ships_from_this_distance == 0) {
            continue;
        }
        
        //Add ships from this planet.
        const int ships_to_send_from_here = std::min(available_ships, max_ships_from_this_distance);
        
        Action* action = Action::Get();
        action->SetOwner(player);
        action->SetSource(source);
        action->SetTarget(target);
        action->SetDistance(distance_to_source);
		action->SetDepartureTime(arrival_time - distance_to_source);
        action->SetNumShips(ships_to_send_from_here);
        
        invasion_plan.push_back(action);

        ships_to_send += ships_to_send_from_here;
        remaining_ships_needed -= ships_to_send_from_here;
        max_ships_from_this_distance -= ships_to_send_from_here;
        remaining_ships_to_take_over = std::max(remaining_ships_to_take_over - ships_to_send_from_here, 0);

		//Check whether this planet would already get enough ships.
        if (remaining_ships_needed <= 0) {
            break;
        }
    } //End iterating over sources

    //Return the invasion plan only if we've found enough ships to do the job.
    if (remaining_ships_needed > 0 || 0 == ships_to_send) {
        Action::FreeActions(invasion_plan);
        invasion_plan.clear();
    }

    return invasion_plan;
}

double Bot::ReturnForMove(const ActionList& invasion_plan, const double best_return) {
    if (invasion_plan.empty()) {
        return 0;
    }
    
    const int horizon = timeline_->Horizon();

    //Calculate the number of ships sent.
    int ships_to_send = 0;
    for (uint i = 0; i < invasion_plan.size(); ++i) {
        ships_to_send += invasion_plan[i]->NumShips();
    }

    //Find a rough upper limit of how good this move could be.
    PlanetTimeline* target = invasion_plan[0]->Target();
    const int arrival_time = invasion_plan[0]->DepartureTime() + invasion_plan[0]->Distance();
    const double multiplier = (kEnemy == target->OwnerAt(arrival_time) ? kAggressionReturnMultiplier : 1);
    const int ships_gained = target->ShipsGainedForActions(invasion_plan);
    const double return_ratio = ReturnRatio(ships_gained, ships_to_send);
    const int my_arrivals = target->MyArrivalsAt(arrival_time);

    //Don't be as restrictive on the first turn.
    const int first_departure_time = invasion_plan[invasion_plan.size() - 1]->DepartureTime(); 
    const int is_first_turn = (turn_ == 1) && (first_departure_time == 0);
    
    if (best_return >= return_ratio && is_first_turn) {
        //The upper limit doesn't beat the best move so far.
        return return_ratio;
    }

    //Perform a more thorough check of the move.  Apply it to the timeline, and see how it
    //impacts ship returns and strategic defense_potentials.
    timeline_->ApplyTempActions(invasion_plan);
    PlanetTimelineList sources_and_targets = Action::SourcesAndTargets(invasion_plan);
    timeline_->UpdatePotentials(sources_and_targets);

    //const int updated_ships_gained = timeline_->ShipsGainedFromBase();
    bool use_min_support = false;
    const int was_neutral = (target->OwnerAt(arrival_time - 1) == kNeutral);
    if (was_neutral) use_min_support = true;

#ifdef LOSE_SHIPS_ONLY_TO_NEUTRALS
    const int neutral_ships = (was_neutral ? target->ShipsAt(arrival_time - 1) : 0);
    const int ships_permanently_lost = std::max(0, neutral_ships - my_arrivals);
    //const int returned_ships = ships_to_send - ships_permanently_lost;
    //pw_assert(returned_ships >= 0 && "Can't regain more ships than were spent");
#else
    const int returned_ships = 0;
    const int ships_permanently_lost = 0;
#endif

    const int updated_ships_gained = 
        timeline_->PotentialShipsGainedForTarget(target, use_min_support) - ships_permanently_lost;
    
    int updated_ships_to_send = ships_to_send;

#ifdef ADD_FUTURE_ENEMY_ARRIVALS_TO_SHIPS_SENT
    int my_cumulative_arrivals = 0;
    
    for (int t = arrival_time + 1; t < horizon; ++t) {
        if (target->PotentialOwnerAt(t) != kMe) {
            break;
        }
        
        my_cumulative_arrivals += target->MyArrivalsAt(t);
        const int enemy_arrivals = target->EnemyArrivalsAt(t);
        const int enemies_to_deal_with = std::max(0, enemy_arrivals - my_cumulative_arrivals);
        my_cumulative_arrivals = std::max(0, my_cumulative_arrivals - enemy_arrivals);
        
        updated_ships_to_send += enemies_to_deal_with;
    }
#endif

    const double updated_return_ratio = ReturnRatio(updated_ships_gained, updated_ships_to_send) * multiplier;
    
    if (best_return < updated_return_ratio) {
        //It is definitely a good move.
        timeline_->ResetTimelinesToBase();
        return updated_return_ratio;
    }

    timeline_->ResetTimelinesToBase();
    return updated_return_ratio;
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

    PlanetTimelineList timelines = timeline_->Timelines();
    PlanetTimelineList sources = timeline_->TimelinesOwnedBy(player, 0 /*current turn*/);
    PlanetTimelineList opponent_planets = timeline_->TimelinesOwnedBy(opponent, 0);

    for (uint i = 0; i < sources.size(); ++i) {
        PlanetTimeline* source = sources[i];
        const int free_ships = source->ShipsFree(0, player);
        const int source_id = source->Id();
        
        //Work only with those reinforcement targets that are actually owned by the
        //player at the time of feeder fleet arrivals.
        PlanetTimelineList possible_feeding_targets = timeline_->EverOwnedTimelinesByDistance(player, source);
        PlanetTimelineList feeding_targets;
        feeding_targets.reserve(possible_feeding_targets.size());

        for (uint j = 0; j < possible_feeding_targets.size(); ++j) {
            PlanetTimeline* possible_target = possible_feeding_targets[j];
            const int distance_to_target = game_->GetDistance(source_id, possible_target->Id());

            if (possible_target->OwnerAt(distance_to_target) == player) {
                feeding_targets.push_back(possible_target);
            }
        }

        //Check whether the player owns any planets closer to enemy
        //planets than this one.
        //PlanetList player_planets_by_distance = game_->PlayerPlanetsByDistance(player, planet);
        PlanetList opponents_by_distance = game_->PlayerPlanetsByDistance(opponent, source_id);
        //PlanetList targets_by_distance = game_->NotPlayerPlanetsByDistance(player, planet);

        pw_assert(!opponents_by_distance.empty());

        //Find the closest non-zero growth planet owned by the opponent.
        Planet* closest_opponent = NULL;
        for (uint j = 0; j < opponents_by_distance.size(); ++j) {
            if (opponents_by_distance[j]->GrowthRate() > 0) {
                closest_opponent = opponents_by_distance[j];
                break;
            }
        }

        if (NULL == closest_opponent) {
            break;
        }

        const int distance_to_opponent = game_->GetDistance(source_id, closest_opponent->Id());
        const int closest_opponent_id = closest_opponent->Id();

        //Find the best allied planet to reinforce.
        int send_ships_to = -1;
        int shortest_distance_to_opponent = distance_to_opponent;
        int distance_to_target = 0;

        for (uint p = 1; p < feeding_targets.size(); ++p) {
            PlanetTimeline* possible_target = feeding_targets[p];
            const int possible_target_id = possible_target->Id();
            const int target_distance_to_opponent = game_->GetDistance(possible_target_id, closest_opponent_id);
            const int target_distance_to_source = game_->GetDistance(possible_target_id, source_id);

            if (target_distance_to_opponent < shortest_distance_to_opponent
              && (distance_to_opponent - distance_threshold) > target_distance_to_source) {
                shortest_distance_to_opponent = target_distance_to_opponent;
                send_ships_to = possible_target_id;
                distance_to_target = target_distance_to_source;
            }
        }
        

        if (-1 != send_ships_to) {
            //Find the number of ships this planet can send away.  The planet cannot send
            //ships that have been reserved for other purposes, and cannot put itself into 
            //negative strategic balance at any point in the future.
            int min_min_balance = source->ShipsFree(0, player);
            const int closest_planet_id = game_->ClosestPlanet(source_id);
            const int distance_to_closest_planet = game_->GetDistance(source_id, closest_planet_id);

            for (int t = distance_to_closest_planet; t < horizon; ++t) {
                const int min_defense_potential = source->MinDefensePotentialAt(t);

                if (min_min_balance > min_defense_potential) {
                    min_min_balance = min_defense_potential;
                }
            }

            if (min_min_balance <= 0) {
                continue;   //To the next source planet.
            }

            const int available_ships = min_min_balance;

            //Send the ships to the planet and see if it negatively impacts the balance.
            PlanetTimeline* target = timelines[send_ships_to];

            Action* action = Action::Get();
            action->SetOwner(player);
            action->SetSource(source);
            action->SetTarget(target);
            action->SetDepartureTime(0);
            action->SetDistance(distance_to_target);
            action->SetNumShips(available_ships);
            temp_action_list[0] = action;

            timeline_->ApplyTempActions(temp_action_list);

            bool was_action_accepted = true;

            if (timeline_->HasSupportWorsenedFor(sources)) {
                //Try the same thing, but with only 2x the source's growth rate.
                timeline_->ResetTimelinesToBase();
                const int ships_to_send = source->GetPlanet()->GrowthRate() * 2;
                action->SetNumShips(ships_to_send);
                timeline_->ApplyTempActions(temp_action_list);

                if (timeline_->HasSupportWorsenedFor(sources)) {
                    was_action_accepted = false;
                }
            }

            if (was_action_accepted) {
                timeline_->SaveTimelinesToBase();
                reinforcing_fleets.push_back(action);
            
            } else {
                action->Free();
                timeline_->ResetTimelinesToBase();
            }


            source->SetReinforcer(true);
        }
    }
    
    timeline_->SaveTimelinesToBase();


    return reinforcing_fleets;
}

void Bot::MarkReinforcers(const int player) {
    ActionList reinforcing_fleets;
    const int horizon = timeline_->Horizon();
    const int distance_threshold = 1;
    const int opponent = OtherPlayer(player);
    
    if (game_->PlanetsOwnedBy(opponent).empty()) {
        return;
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
            planet_timeline->SetReinforcer(true);
        }
    }
    
    timeline_->UpdatePotentials();
    timeline_->SaveTimelinesToBase();
}

ActionList Bot::SendSupportFleets(const int player) {
    ActionList support_actions;
    const int horizon = timeline_->Horizon();

    PlanetTimelineList targets = timeline_->EverOwnedTimelines(player);
    if (targets.empty()) {
        return support_actions;
    }

    PlanetTimelineList test_planets = timeline_->TimelinesOwnedBy(player, 0);

    for (uint i = 0; i < targets.size(); ++i) {
        PlanetTimeline* target = targets[i];
        const int target_id = target->Id();

        //Planets that might be participating in the invasion, sorted by distance from target.
		PlanetTimelineList sources = timeline_->EverOwnedTimelinesByDistance(player, target);

        if (sources.empty()) {
            continue;   //To the next arrival time.
        }
        
        //Pre-calculate distances to the sources.
		std::vector<int> distances_to_sources(sources.size());

		for (uint s = 0; s < sources.size(); ++s) {
			distances_to_sources[s] = game_->GetDistance(sources[s]->Id(), target_id);
		}

		//Find earliest time the fleet can reach the target.
        const int earliest_arrival = distances_to_sources[0];
        
#ifndef IS_SUBMISSION
        if (1 == picking_round_ && 11 == target_id) {
            int x = 2;
        }
#endif
        for (int arrival_time = distances_to_sources[0]; arrival_time < horizon; ++arrival_time) {
            if (target->OwnerAt(arrival_time) != player) {
                continue;
            }

            //Check whether there is a need for support.
            const int min_defense_potential = target->MinDefensePotentialAt(arrival_time);
            if (min_defense_potential >= 0 || min_defense_potential < -kSupportShipLimit) {
                continue;
            }

            //Find the supporting actions.
            ActionList support_plan = this->FindInvasionPlan(target, arrival_time, sources, distances_to_sources, player);

            if (support_plan.empty()) {
                continue;
            }

            //Check whether they cause other planets to lose support.
            timeline_->ApplyTempActions(support_plan);
            timeline_->UpdatePotentials(support_plan);

            if (timeline_->HasSupportWorsenedFor(test_planets)) {
            //if (this->ReturnForMove(support_plan, 0) <= 0) {
                //Bad plan, don't do it.
                Action::FreeActions(support_plan);
                timeline_->ResetTimelinesToBase();

            } else {
                //Good plan, go ahead.
                //this->AddSupportConstraints(support_plan);
                timeline_->SaveTimelinesToBase();
                //this->ApplyActions(support_plan);
                
                for (uint j = 0; j < support_plan.size(); ++j) {
                    support_actions.push_back(support_plan[j]);
                }
            }
        } //End iterating over arrival times.
    } //End iterating over targets.

    return support_actions;
}

void Bot::ApplyActions(const ActionList& actions) {
    if (actions.empty()) {
        return;
    }
    
    //Check whether the action has caused any planets to lose support.
    //If that's the case, set the constraints on support of those planets.
    timeline_->ApplyTempActions(actions);
    timeline_->UpdatePotentials(actions);

#ifdef USE_SUPPORT_CONSTRAINTS
    this->AddSupportConstraints(actions);
#endif

    timeline_->SaveTimelinesToBase();
}

void Bot::AddSupportConstraints(const ActionList &actions) {
    //Assumes that the actions have already been temporarily applied to the working timelines.
    if (actions.empty()) {
        return;
    }
    
    const int player = actions[0]->Owner();
    PlanetTimeline* target = actions[0]->Target();
    PlanetTimelineList my_planets = timeline_->EverOwnedTimelines(player);

    for (uint i = 0; i < my_planets.size(); ++i) {
        PlanetTimeline* planet = my_planets[i];

        if (timeline_->HasSupportWorsenedFor(planet)) {
            support_constraints_->AddConstraint(planet, target);
        }
    }
}

/************************************************
            SupportConstraints class
************************************************/
SupportConstraints::SupportConstraints(const int num_planets, GameMap* game) {
    const uint u_num_planets = static_cast<uint>(num_planets);
    constraint_centers_.resize(u_num_planets);
    constraint_radii_.resize(u_num_planets);
    game_ = game;
}

void SupportConstraints::AddConstraint(PlanetTimeline *constrained_planet, PlanetTimeline *constraint_center) {
    const int constrainee_id = constrained_planet->Id();
    const int center_id = constraint_center->Id();
    const int radius = game_->GetDistance(center_id, constrainee_id);

    constraint_centers_[constrainee_id].push_back(center_id);
    constraint_radii_[constrainee_id].push_back(radius);
}

bool SupportConstraints::MaySupport(PlanetTimeline *source, PlanetTimeline *target) {
    const int source_id = source->Id();
    const int target_id = target->Id();

    for(uint i = 0; i < constraint_centers_[target_id].size(); ++i) {
        const int distance_from_center = game_->GetDistance(constraint_centers_[target_id][i], source_id);
        const int minimum_distance = constraint_radii_[target_id][i];

        if (distance_from_center <= minimum_distance) {
            return false;
        }
    }

    return true;
}

void SupportConstraints::ClearConstraints() {
    for (uint i = 0; i < constraint_centers_.size(); ++i) {
        constraint_centers_[i].clear();
        constraint_radii_[i].clear();
    }
}
