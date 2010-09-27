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
    
    //Find my best response to enemy's best response to my best moves.
 //   ActionList my_actions = this->FindActionsFor(kMe);
    ActionList enemy_actions = this->FindActionsFor(kEnemy);
//    timeline_->UnapplyActions(my_actions);
//    Action::FreeActions(my_actions);

    ActionList my_best_actions = this->FindActionsFor(kMe);

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

    while (invadeable_planets.size() != 0) {
        ActionList best_actions = this->BestRemainingMove(invadeable_planets, player);

        if (best_actions.empty()) {
            break;
        }

        //Add these actions to the list of other actions.
        for (uint i = 0; i < best_actions.size(); ++i) {
            player_actions.push_back(best_actions[i]);
        }
        
        timeline_->ApplyActions(best_actions);
    }

    return player_actions;
}

ActionList Bot::BestRemainingMove(PlanetTimelineList &invadeable_planets, const int player) {
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
		
		//Find earliest time the fleet can reach the target.
		const int earliest_arrival = distances_to_sources[first_source];

		for (int t = earliest_arrival; t < horizon; ++t) {
			//Find a possible invasion fleet, calculate the return on sending it.
			const int ships_needed = target->ShipsRequredToPosess(t, player);
            if (0 == ships_needed) {
                continue;
            }

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
                continue;

            } else {
                const int ships_gained = target->ShipsGainedForActions(invasion_plan);
                const double return_ratio = static_cast<double>(ships_gained)
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
		} //End iterating over arrival times.
	}//End iterating over targets.

	return best_actions;
}

