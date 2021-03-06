//Available under GPLv3.
//Author: Iouri Khramtsov.

#include "GameTimeline.h"
#include "Utils.h"

/************************************************
               GameTimeline class
************************************************/
GameTimeline::GameTimeline()
: game_(NULL) {
}

GameTimeline::~GameTimeline() {
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        delete planet_timelines_[i];
        delete base_planet_timelines_[i];
    }
}

void GameTimeline::SetGameMap(GameMap* game) {
    game_ = game;
    horizon_ = game->MapRadius() + 5;

    //Update the ships gained from planets vector.
    PlanetList planets = game->Planets();
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = new PlanetTimeline();
        timeline->Initialize(horizon_, planets[i], game, this);
        planet_timelines_.push_back(timeline);
    }

    this->UpdateBalances();

    //Initialize the base planet timelines.
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* base_timeline = new PlanetTimeline();
        base_timeline->Copy(planet_timelines_[i]);
        base_planet_timelines_.push_back(base_timeline);
    }

    //Initialize indicators showing whether the base timeline is different
    //from the working timeline.
    are_working_timelines_different_.resize(planet_timelines_.size(), false);
}

void GameTimeline::Update() {
    //Update the planet data.
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        planet_timelines_[i]->Update();
        are_working_timelines_different_[i] = true;
    }

    this->UpdateBalances();
    this->SaveTimelinesToBase();
}

int GameTimeline::ShipsGainedForActions(const ActionList& actions, Planet *planet) const {
    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
    const int ships_gained = timeline->ShipsGainedForActions(actions);
    return ships_gained;
}

int GameTimeline::ShipsGainedFromBase() const {
    int ships_gained = 0;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        ships_gained += (planet_timelines_[i]->ShipsGained() - base_planet_timelines_[i]->ShipsGained());
    }

    return ships_gained;
}

PlanetTimeline* GameTimeline::HighestShipLossTimeline() {
    int worst_ship_loss = 0;
    PlanetTimeline* worst_timeline = NULL;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        const int ships_gained = planet_timelines_[i]->ShipsGained() - base_planet_timelines_[i]->ShipsGained();
        if (worst_ship_loss > ships_gained) {
            worst_ship_loss = ships_gained;
            worst_timeline = planet_timelines_[i];
        }
    }

    return worst_timeline;
}

PlanetList GameTimeline::PlanetsThatWillNotBeMine() const {
    PlanetList will_not_be_mine;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->WillNotBeMine()) {
            will_not_be_mine.push_back(timeline->GetPlanet());
        }
    }

    return will_not_be_mine;
}

PlanetTimelineList GameTimeline::TimelinesEverNotOwnedBy(const int player) const {
    PlanetTimelineList will_not_be_owned;
    
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];
        
        if (timeline->WillNotBeOwnedBy(player)) {
            will_not_be_owned.push_back(timeline);
        }
    }

    return will_not_be_owned;
}

int GameTimeline::ShipsRequredToPosess(Planet* planet, int arrival_time, int by_whom) const {
    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
    const int ships_needed = timeline->ShipsRequredToPosess(arrival_time, by_whom);
    return ships_needed;
}

PlanetTimelineList GameTimeline::TimelinesOwnedBy(int owner, int when) const {
    PlanetTimelineList timelines;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->IsOwnedBy(owner, when)) {
            timelines.push_back(timeline);
        }
    }

    return timelines;
}

PlanetTimelineList GameTimeline::TimelinesNotOwnedBy(int owner, int when) const {
    PlanetTimelineList timelines;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (!timeline->IsOwnedBy(owner, when)) {
            timelines.push_back(timeline);
        }
    }

    return timelines;
}

PlanetTimelineList GameTimeline::EverOwnedTimelines(const int owner) {
    PlanetTimelineList timelines;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->WillBeOwnedBy(owner)) {
            timelines.push_back(timeline);
        }
    }

    return timelines;
}

PlanetTimelineList GameTimeline::EverNotOwnedTimelines(int owner) {
    PlanetTimelineList timelines;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->WillNotBeOwnedBy(owner)) {
            timelines.push_back(timeline);
        }
    }

    return timelines;
}

PlanetTimelineList GameTimeline::EverNotOwnedNonReinforcerTimelines(int owner) {
    PlanetTimelineList timelines;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->WillNotBeOwnedBy(owner) && !timeline->IsReinforcer()) {
            timelines.push_back(timeline);
        }
    }

    return timelines;
}

PlanetTimelineList GameTimeline::EverOwnedTimelinesByDistance(int owner, PlanetTimeline *source) {
    PlanetList planets = game_->PlanetsByDistance(source->Id());
    PlanetTimelineList timelines;
    
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[planets[i]->Id()];

        if (timeline->WillBeOwnedBy(owner)) {
            timelines.push_back(timeline);
        }
    }
    
    return timelines;
}

PlanetTimelineList GameTimeline::OwnedTimelinesByDistance(int owner, 
                                                                PlanetTimeline* source, 
                                                                int when) {
    PlanetList planets = game_->PlanetsByDistance(source->Id());
    PlanetTimelineList timelines;
    
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[planets[i]->Id()];

        if (timeline->IsOwnedBy(owner, when)) {
            timelines.push_back(timeline);
        }
    }
    
    return timelines;
}

PlanetTimelineList GameTimeline::TimelinesByDistance(PlanetTimeline* source) {
    PlanetList planets = game_->PlanetsByDistance(source->Id());
    PlanetTimelineList timelines;
    
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[planets[i]->Id()];
        timelines.push_back(timeline);
    }
    
    return timelines;
}

void GameTimeline::ApplyActions(const ActionList& actions) {
	if (actions.empty()) {
		return;
	}

    this->ApplyTempActions(actions);
    PlanetTimelineList sources_and_targets = Action::SourcesAndTargets(actions);
    this->UpdateBalances();
    this->SaveTimelinesToBase();

#ifndef IS_SUBMISSION
    this->AssertWorkingTimelinesAreEqualToBase();
#endif
    //this->UpdateBalances(sources_and_targets);
    //this->UpdateBalances();
#ifndef IS_SUBMISSION
    this->AssertWorkingTimelinesAreEqualToBase();
#endif

    //this->UpdateBalances(sources_and_targets);

    
    //for (uint i = 0; i < are_working_timelines_different_.size(); ++i) {
    //    if (are_working_timelines_different_[i]) {
    //        base_planet_timelines_[i]->CopyTimeline(planet_timelines_[i]);
    //        are_working_timelines_different_[i] = false;
    //    }

    //    base_planet_timelines_[i]->CopyBalances(planet_timelines_[i]);
    //}

#ifndef IS_SUBMISSION
    this->AssertWorkingTimelinesAreEqualToBase();
#endif
}

void GameTimeline::ApplyTempActions(const ActionList& actions) {
	if (actions.empty()) {
		return;
	}
	
	//Assumption: actions are grouped by their target.
	
	//Apply the actions to their target.
    PlanetTimelineList targets;
    std::vector<ActionList> actions_for_targets;

    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        PlanetTimeline* target = action->Target();
        bool found = false;

        for (uint j = 0; j < targets.size(); ++j) {
            if (targets[j] == target) {
                actions_for_targets[j].push_back(action);

                found = true;
            }
        }

        if (!found) {
            targets.push_back(target);
            ActionList actions_for_this_target;
            actions_for_this_target.push_back(action);
            actions_for_targets.push_back(actions_for_this_target);
        }
    }

    for (uint i = 0; i < targets.size(); ++i) {
        targets[i]->AddArrivals(actions_for_targets[i]);
        are_working_timelines_different_[targets[i]->Id()] = true;
    }

	//PlanetTimeline* target = actions[0]->Target();
	//target->AddArrivals(actions);
    //are_working_timelines_different_[target->Id()] = true;

	//Apply the actions to their sources.
	for (uint i = 0; i < actions.size(); ++i) {
		//pw_assert(target->Id() == actions[i]->Target()->Id() && "Applied actions must have same target");

		PlanetTimeline* source = actions[i]->Source();
		source->AddDeparture(actions[i]);
        are_working_timelines_different_[source->Id()] = true;
	}
}

void GameTimeline::UnapplyActions(const ActionList &actions) {
    if (actions.empty()) {
        return;
    }

    std::vector<bool> is_timeline_affected;
    is_timeline_affected.resize(planet_timelines_.size(), false);
    
    //Remove arrivals and departures associated with the actions from the affected timelines.
    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        PlanetTimeline* source = action->Source();
        PlanetTimeline* target = action->Target();

        source->RemoveDeparture(action);
        target->RemoveArrival(action);

        is_timeline_affected[source->Id()] = true;
        is_timeline_affected[target->Id()] = true;
    }

    //Recalculate the affected timelines.
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        if (is_timeline_affected[i]) {
            PlanetTimeline* timeline = planet_timelines_[i];
            timeline->ResetStartingData();
            timeline->RecalculateTimeline(1);
        }
    }
}

void GameTimeline::ResetTimelinesToBase() {
    for (uint i = 0; i < are_working_timelines_different_.size(); ++i) {
        if (are_working_timelines_different_[i]) {
            planet_timelines_[i]->CopyTimeline(base_planet_timelines_[i]);
            are_working_timelines_different_[i] = false;
        }

        planet_timelines_[i]->CopyBalances(base_planet_timelines_[i]);
    }

#ifndef IS_SUBMISSION
    this->AssertWorkingTimelinesAreEqualToBase();
#endif
}

void GameTimeline::SaveTimelinesToBase() {
    for (uint i = 0; i < are_working_timelines_different_.size(); ++i) {
        if (are_working_timelines_different_[i]) {
            base_planet_timelines_[i]->CopyTimeline(planet_timelines_[i]);
            are_working_timelines_different_[i] = false;
        }

        base_planet_timelines_[i]->CopyBalances(planet_timelines_[i]);
    }

#ifndef IS_SUBMISSION
    this->AssertWorkingTimelinesAreEqualToBase();
#endif
}

void GameTimeline::MarkTimelineAsModified(int timeline_id) {
    are_working_timelines_different_[timeline_id] = true;
}

int GameTimeline::NegativeBalanceImprovement() {
    int total_improvement = 0;
    
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        const int base_negative_balance = base_planet_timelines_[i]->TotalNegativeMinBalance();
        const int working_negative_balance = planet_timelines_[i]->TotalNegativeMinBalance();
        const int improvement = base_negative_balance - working_negative_balance;
        total_improvement += improvement;
    }

    return total_improvement;
}

bool GameTimeline::HasNegativeBalanceWorsenedFor(PlanetTimelineList timelines) {
    for (uint i = 0; i < timelines.size(); ++i) {
        PlanetTimeline* timeline = timelines[i];
        PlanetTimeline* base_timeline = base_planet_timelines_[timeline->Id()];
        
        for (int t = 0; t < horizon_; ++t) {
            const int min_balance = timeline->MinBalanceAt(t);
            if (min_balance < 0 && min_balance < base_timeline->MinBalanceAt(t)) {
                return true;
            }
        }
    }

    return false;
}

void GameTimeline::UpdateBalances(const int depth) {
    //Calculate the strategic balances at each turn for each planet.
    //A strategic balance for turn t and distance d is the sum of my ships that can reach the planet
    //at t departing after turn (t-d) minus the enemy ships that can reach the planet on turn t 
    //departing on or after turn (t-d).
    //Positive balances are good; negative balances are bad.
    const std::vector<int>& when_is_feeder_allowed_to_attack = *when_is_feeder_allowed_to_attack_;
    const int num_planets = game_->NumPlanets();

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* planet = planet_timelines_[i];
        PlanetTimelineList planets_by_distance = this->TimelinesByDistance(planet);
        std::vector<int>& balances = planet->Balances();
        int first_negative_min_balance = horizon_;
        int first_positive_max_balance = horizon_;
        int total_negative_min_balance = 0;
        const int first_source_distance = game_->GetDistance(planets_by_distance[0]->Id(), planet->Id());
        const int first_t = first_source_distance;

#ifndef IS_SUBMISSION
        const int id = planet->Id();
#endif

        for (int t = 1; t < horizon_; ++t) {
#ifndef IS_SUBMISSION
            if (15 == id && 7 == t) {
                int x = 2;
            }
#endif
            //Calculate the planet's own contribution to the balances.
            const int planet_owner = planet->OwnerAt(t);
            const int prev_owner = planet->OwnerAt(t - 1);
            const int neutral_adjustment = planet->NeutralBalanceAdjustment(t);
            const int starting_balance = planet->ShipsAt(t) * (planet_owner == kMe ? 1 : -1) + neutral_adjustment;

            const int offset = t * (t - 1) / 2 - 1;
            int min_balance = starting_balance;
            int max_balance = starting_balance;
           
            for (int d = 1; d <= t; ++d) {
                balances[offset + d] = starting_balance;
            }

            //Calculate the neighbours' contributions to the balances.
            for (uint s = 0; s < planets_by_distance.size(); ++s) {
                PlanetTimeline* source = planets_by_distance[s];
                
                const int distance_to_source = game_->GetDistance(source->Id(), planet->Id());

                if (distance_to_source > t) {
                    break;
                }

                if (source->IsReinforcer() && planet_owner == kEnemy && prev_owner != kMe) {
                    //Feeder planets aren't allowed to attack enemies unless explicitly allowed.
                    const int attack_permission_index = num_planets * source->Id() + planet->Id();
                    if (when_is_feeder_allowed_to_attack[attack_permission_index] != (t - distance_to_source)) {
                        continue;
                    }
                }
                //if (source->IsReinforcer()) {
                //    continue;
                //}

                const int owner = source->OwnerAt(t - distance_to_source);

                const int ships = source->ShipsFree(t - distance_to_source, owner);
                const int ships_from_source = OwnerMultiplier(owner) * ships;
                const int first_d = distance_to_source + (kMe == owner ? 1 : 0);

                for (int d = first_d; d <= t; ++d) {
                    balances[offset + d] += ships_from_source;
                }
            }

            //Calculate the summaries.
            for (int d = first_source_distance; d <= t; ++d) {
                const int index = offset + d;

                if (min_balance > balances[index]) {
                    min_balance = balances[index];
                }

                if (max_balance < balances[index]) {
                    max_balance = balances[index];
                }
            }

            //Update min/max balances at each turn.
            planet->SetMinBalanceAt(t, min_balance);
            planet->SetMaxBalanceAt(t, max_balance);

            if (min_balance < 0 && first_negative_min_balance == horizon_) {
                first_negative_min_balance = t;

            } else if (max_balance > 0 && first_positive_max_balance == horizon_) {
                first_positive_max_balance = t;

            }

            if (min_balance < 0) {
                total_negative_min_balance += min_balance;
            }

        }

        planet->SetFirstNegativeMinBalanceTurn(first_negative_min_balance);
        planet->SetFirstPositiveMaxBalanceTurn(first_positive_max_balance);
        planet->SetTotalNegativeMinBalance(-total_negative_min_balance);
    }

    //Update the timelines' ships gained.
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        planet_timelines_[i]->RecalculateShipsGained();
    }
}

void GameTimeline::UpdateBalances(const PlanetTimelineList& modified_planets, const int depth) {
    //Update balances.  Update only the effects of the planets whose timelines have been changed.
    const std::vector<int>& when_is_feeder_allowed_to_attack = *when_is_feeder_allowed_to_attack_;
    const int num_planets = game_->NumPlanets();

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* planet = planet_timelines_[i];
        PlanetTimeline* base_planet = base_planet_timelines_[i];
        PlanetTimelineList planets_by_distance = this->TimelinesByDistance(planet);
        std::vector<int>& balances = planet->Balances();
        int first_negative_min_balance = horizon_;
        int first_positive_max_balance = horizon_;
        int total_negative_min_balance = 0;
        const int first_source_distance = game_->GetDistance(planets_by_distance[0]->Id(), planet->Id());
        const int first_t = first_source_distance;

#ifndef IS_SUBMISSION
        const int id = planet->Id();
#endif

        for (int t = 1; t < horizon_; ++t) {
#ifndef IS_SUBMISSION
            if (22 == id && 23 == t) {
                int x = 2;
            }
#endif
            //Calculate the planet's own contribution to the balances.
            const int planet_owner = planet->OwnerAt(t);
            const int prev_planet_owner = planet->OwnerAt(t - 1);
            const int starting_balance = planet->ShipsAt(t) * (planet_owner == kMe ? 1 : -1);
            const int base_planet_owner = base_planet->OwnerAt(t);
            const int base_prev_planet_owner = base_planet->OwnerAt(t - 1);
            const int base_starting_balance = base_planet->ShipsAt(t) * (base_planet_owner == kMe ? 1 : -1);

            const int neutral_adjustment = planet->NeutralBalanceAdjustment(t);
            const int base_neutral_adjustment = base_planet->NeutralBalanceAdjustment(t);
            const int neutral_adjustment_change = neutral_adjustment - base_neutral_adjustment;

            const int starting_balance_diff = starting_balance - base_starting_balance + neutral_adjustment_change;


            const int offset = t * (t - 1) / 2 - 1;
            int min_balance = starting_balance;
            int max_balance = starting_balance;
            
            if (starting_balance_diff != 0) {
                for (int d = 1; d <= t; ++d) {
                    balances[offset + d] += starting_balance_diff;
                }
            }

            //Calculate the neighbours' contributions to the balances.
            for (uint s = 0; s < planets_by_distance.size(); ++s) {
                PlanetTimeline* source = planets_by_distance[s];
                const int source_id = source->Id();
                
                const int distance_to_source = game_->GetDistance(source->Id(), planet->Id());
                
                if (distance_to_source > t) {
                    break;
                }

                //Use only the sources from the provided list.
                bool found_source = false;
                for (uint j = 0; j < modified_planets.size(); ++j) {
                    if (modified_planets[j]->Id() == source_id) {
                        found_source = true;
                        break;
                    }
                }

                if (!found_source) {
                    continue;
                }

                //Calculate how much has the effect of this source changed.
                PlanetTimeline* base_source = base_planet_timelines_[source->Id()];
                const int owner = source->OwnerAt(t - distance_to_source);
                const int base_owner = base_source->OwnerAt(t - distance_to_source);
                int source_effect = 1;
                int base_source_effect = 1;

                if (source->IsReinforcer() && (planet_owner == kEnemy || base_planet_owner == kEnemy)) {
                    //Feeder planets aren't allowed to attack enemies unless explicitly allowed.
                    const int attack_permission_index = num_planets * source->Id() + planet->Id();

                    if (when_is_feeder_allowed_to_attack[attack_permission_index] != (t - distance_to_source)) {
                        if (base_planet_owner == kEnemy && base_prev_planet_owner != kMe) base_source_effect = 0;
                        if (planet_owner == kEnemy && prev_planet_owner != kMe) source_effect = 0;
                    }
                }
                //if (source->IsReinforcer()) {
                //    continue;
                //}

                const int ships = source->ShipsFree(t - distance_to_source, owner);
                const int ships_from_source = OwnerMultiplier(owner) * ships * source_effect;
                const int base_ships = base_source->ShipsFree(t - distance_to_source, base_owner);
                const int base_ships_from_source = OwnerMultiplier(base_owner) * base_ships * base_source_effect;
                const int balance_change = ships_from_source - base_ships_from_source;

                //Apply the changes.
                const int first_d = distance_to_source + (kMe == owner || kMe == base_owner ? 1 : 0);

                if (first_d != distance_to_source) {
                    const int multiplier = (kMe == owner ? 0 : 1);
                    const int base_multiplier = (kMe == base_owner ? 0 : 1);
                    const int change = ships_from_source * multiplier - base_ships_from_source * base_multiplier;
                    balances[offset + distance_to_source] += change;
                }

                if (0 == balance_change) {
                    continue;
                }

                for (int d = first_d; d <= t; ++d) {
                    balances[offset + d] += balance_change;
                }
            }

            //Calculate the summaries.
            for (int d = first_source_distance; d <= t; ++d) {
                const int index = offset + d;

                if (min_balance > balances[index]) {
                    min_balance = balances[index];
                }

                if (max_balance < balances[index]) {
                    max_balance = balances[index];
                }
            }

            //Update min/max balances at each turn.
            planet->SetMinBalanceAt(t, min_balance);
            planet->SetMaxBalanceAt(t, max_balance);

            if (min_balance < 0 && first_negative_min_balance == horizon_) {
                first_negative_min_balance = t;

            } else if (max_balance > 0 && first_positive_max_balance == horizon_) {
                first_positive_max_balance = t;

            }

            if (min_balance < 0) {
                total_negative_min_balance += min_balance;
            }

        }

        planet->SetFirstNegativeMinBalanceTurn(first_negative_min_balance);
        planet->SetFirstPositiveMaxBalanceTurn(first_positive_max_balance);
        planet->SetTotalNegativeMinBalance(-total_negative_min_balance);
    }
    
    //Update the timelines' ships gained.
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        planet_timelines_[i]->RecalculateShipsGained();
    }
}

#ifndef IS_SUBMISSION
void GameTimeline::AssertWorkingTimelinesAreEqualToBase() {
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        pw_assert(planet_timelines_[i]->Equals(base_planet_timelines_[i]) && "Working Timelines are different from base.");
    }
}
#endif

/************************************************
               PlanetTimeline class
************************************************/
PlanetTimeline::PlanetTimeline()
:game_(NULL), planet_(NULL), is_reinforcer_(false), is_recalculating_(false) {
}

void PlanetTimeline::Initialize(int forecast_horizon, Planet *planet, GameMap *game, GameTimeline* game_timeline) {
    horizon_ = forecast_horizon;
    planet_ = planet;
    game_ = game;
    game_timeline_ = game_timeline;

    //Initialize the forecast arrays.
    const size_t u_horizon = static_cast<size_t>(horizon_);
    const int growth_rate = planet->GrowthRate();

    id_ = planet->Id();
    owner_.resize(u_horizon, 0);
    ships_.resize(u_horizon, 0);
    my_arrivals_.resize(u_horizon, 0);
    enemy_arrivals_.resize(u_horizon, 0);
    ships_to_take_over_.resize(u_horizon, 0);
    ships_gained_.resize(u_horizon, 0);
    available_growth_.resize(u_horizon, growth_rate);
    ships_reserved_.resize(u_horizon, 0);
    ships_free_.resize(u_horizon, 0);

    enemy_ships_to_take_over_.resize(u_horizon, 0);
    enemy_ships_reserved_.resize(u_horizon, 0);
    enemy_ships_free_.resize(u_horizon, 0);
    enemy_available_growth_.resize(u_horizon, growth_rate);

    my_departures_.resize(u_horizon, 0);
    enemy_departures_.resize(u_horizon, 0);

    my_unreserved_arrivals_.resize(u_horizon, 0);

    my_contingent_departures_.resize(u_horizon, 0);
    enemy_contingent_departures_.resize(u_horizon, 0);
    
    balances_.resize(u_horizon*(u_horizon + 1)/2, 0);
    min_balances_.resize(u_horizon, 0);
    max_balances_.resize(u_horizon, 0);
    
    will_not_be_mine_ = false;
	will_be_mine_ = false;
	will_be_enemys_ = false;
    
    additional_arrivals_.resize(u_horizon, 0);

    //Find out when various ships will be arriving.
    FleetList arrivingFleets = game->FleetsArrivingAt(planet);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];

        if (fleet->Owner() == kMe) {
            my_arrivals_[fleet->TurnsRemaining()] += fleet->NumShips();

        } else {
            enemy_arrivals_[fleet->TurnsRemaining()] += fleet->NumShips();
        }
    }
    
    const int starting_owner = planet->Owner();
	const int num_ships = planet_->NumShips();
    owner_[0] = starting_owner;
    ships_[0] = num_ships;
	ships_free_[0] = (starting_owner == kMe ? num_ships : 0);
	enemy_ships_free_[0] = (starting_owner == kEnemy ? num_ships : 0);
    
    ships_gained_[0] = growth_rate * OwnerMultiplier(starting_owner);
    will_not_be_mine_ = (kMe != starting_owner);
    will_be_mine_ = (kMe == starting_owner);
    will_not_be_enemys_ = (kEnemy != starting_owner);
    will_be_enemys_ = (kEnemy == starting_owner);

    is_recalculating_ = false;

    this->RecalculateTimeline(1);
}

void PlanetTimeline::Copy(PlanetTimeline* other) {
    id_ = other->id_;
    horizon_ = other->horizon_;

    game_ = other->game_;
    planet_ = other->planet_;
    game_timeline_ = other->game_timeline_;
    
    this->CopyTimeline(other);
}

void PlanetTimeline::CopyTimeline(PlanetTimeline* other) {
    owner_ = other->owner_;
    ships_ = other->ships_;
    my_arrivals_ = other->my_arrivals_;
    enemy_arrivals_ = other->enemy_arrivals_;
    ships_to_take_over_ = other->ships_to_take_over_;
    ships_gained_ = other->ships_gained_;
    available_growth_ = other->available_growth_;
    ships_reserved_ = other->ships_reserved_;
    ships_free_ = other->ships_free_;

    enemy_ships_to_take_over_ = other->enemy_ships_to_take_over_;
    enemy_ships_reserved_ = other->enemy_ships_reserved_;
    enemy_ships_free_ = other->enemy_ships_free_;
    enemy_available_growth_ = other->enemy_available_growth_;
    
    my_departures_ = other->my_departures_;
    enemy_departures_ = other->enemy_departures_;

    my_unreserved_arrivals_ = other->my_unreserved_arrivals_;

    my_contingent_departures_ = other->my_contingent_departures_;
    enemy_contingent_departures_ = other->enemy_contingent_departures_;

    total_ships_gained_ = other->total_ships_gained_;

    departing_actions_ = other->departing_actions_;

    balances_ = other->balances_;
    min_balances_ = other->min_balances_;
    max_balances_ = other->max_balances_;
    first_negative_min_balance_turn_ = other->first_negative_min_balance_turn_;
    first_positive_max_balance_turn_ = other->first_positive_max_balance_turn_;
    total_negative_min_balance_ = other->total_negative_min_balance_;
    has_balance_changed_fate_ = other->has_balance_changed_fate_;

    will_not_be_enemys_ = other->will_not_be_enemys_;
    will_not_be_mine_ = other->will_not_be_mine_;
	will_be_enemys_ = other->will_be_enemys_;
	will_be_mine_ = other->will_be_mine_;

    is_reinforcer_ = other->is_reinforcer_;
}

void PlanetTimeline::CopyBalances(PlanetTimeline* other) {
    balances_ = other->balances_;
    min_balances_ = other->min_balances_;
    max_balances_ = other->max_balances_;
    first_negative_min_balance_turn_ = other->first_negative_min_balance_turn_;
    first_positive_max_balance_turn_ = other->first_positive_max_balance_turn_;
    total_negative_min_balance_ = other->total_negative_min_balance_;
    total_ships_gained_ = other->total_ships_gained_;
}

bool PlanetTimeline::Equals(PlanetTimeline* other) const {
    bool are_equal = true;
    are_equal &= (id_ == other->id_);
    are_equal &= (horizon_ == other->horizon_);

    if (!are_equal) {
        return false;
    }

    for (int t = 0; t < horizon_; ++t) {
        are_equal &= (owner_[t] == other->owner_[t]);
        are_equal &= (ships_[t] == other->ships_[t]);
        are_equal &= (my_arrivals_[t] == other->my_arrivals_[t]);
        are_equal &= (enemy_arrivals_[t] == other->enemy_arrivals_[t]);
        are_equal &= (ships_to_take_over_[t] == other->ships_to_take_over_[t]);
        are_equal &= (ships_gained_[t] == other->ships_gained_[t]);
        are_equal &= (available_growth_[t] == other->available_growth_[t]);
        are_equal &= (ships_reserved_[t] == other->ships_reserved_[t]);
        are_equal &= (ships_free_[t] == other->ships_free_[t]);

        are_equal &= (enemy_ships_to_take_over_[t] == other->enemy_ships_to_take_over_[t]);
        are_equal &= (enemy_ships_reserved_[t] == other->enemy_ships_reserved_[t]);
        are_equal &= (enemy_ships_free_[t] == other->enemy_ships_free_[t]);
        are_equal &= (enemy_available_growth_[t] == other->enemy_available_growth_[t]);
        
        are_equal &= (my_departures_[t] == other->my_departures_[t]);
        are_equal &= (enemy_departures_[t] == other->enemy_departures_[t]);

        are_equal &= (my_unreserved_arrivals_[t] == other->my_unreserved_arrivals_[t]);
        are_equal &= (my_contingent_departures_[t] == other->my_contingent_departures_[t]);
        are_equal &= (enemy_contingent_departures_[t] == other->enemy_contingent_departures_[t]);

        //Strategic balances.
        are_equal &= (min_balances_[t] == other->min_balances_[t]);
        are_equal &= (max_balances_[t] == other->max_balances_[t]);

        if (!are_equal) {
            return false;
        }
    }

    if (departing_actions_.size() != other->departing_actions_.size()) {
        return false;
    }

    for (uint i = 0; i < departing_actions_.size(); ++i) {
        if (departing_actions_[i] != other->departing_actions_[i]) {
            return false;
        }
    }

    for (uint i = 0; i < balances_.size(); ++i) {
        if (balances_[i] != other->balances_[i]) {
            return false;
        }
    }

    are_equal &= (total_ships_gained_ == other->total_ships_gained_);
    are_equal &= (first_negative_min_balance_turn_ == other->first_negative_min_balance_turn_);
    are_equal &= (first_positive_max_balance_turn_ == other->first_positive_max_balance_turn_);
    are_equal &= (total_negative_min_balance_ == other->total_negative_min_balance_);
    are_equal &= (has_balance_changed_fate_ == other->has_balance_changed_fate_);

    are_equal &= (will_not_be_enemys_ == other->will_not_be_enemys_);
    are_equal &= (will_not_be_mine_ == other->will_not_be_mine_);
    are_equal &= (will_be_enemys_ == other->will_be_enemys_);
    are_equal &= (will_be_mine_ == other->will_be_mine_);


    are_equal &= (game_ == other->game_);
    are_equal &= (planet_ == other->planet_);
    are_equal &= (game_timeline_ == other->game_timeline_);

    are_equal &= (is_reinforcer_ == other->is_reinforcer_);
    are_equal &= (is_recalculating_ == other->is_recalculating_);
    
    if (!are_equal) {
        return false;
    }

    return are_equal;
}

void PlanetTimeline::Update() {
    //Advance the forecast frame through time.
    is_reinforcer_ = false;

    //Reset the various arrays.
    //For now deal only with my arrivals.
    for (uint i = 0; i < additional_arrivals_.size(); ++i) {
        additional_arrivals_[i] = 0;
        ships_reserved_[i] = 0;
        enemy_ships_reserved_[i] = 0;
        ships_free_[i] = 0;
        enemy_ships_free_[i] = 0;
        my_departures_[i] = 0;
        enemy_departures_[i] = 0;
        available_growth_[i] = 0;
        enemy_available_growth_[i] = 0;
		my_arrivals_[i] = 0;
		enemy_arrivals_[i] = 0;
        my_unreserved_arrivals_[i] = 0;
        my_contingent_departures_[i] = 0;
        enemy_contingent_departures_[i] = 0;
    }

    //Update the fleet arrivals.
    FleetList arrivingFleets = game_->FleetsArrivingAt(planet_);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];
        const int turns_remaining = fleet->TurnsRemaining();
        
        if (fleet->Owner() == kMe) {
            my_arrivals_[turns_remaining] += fleet->NumShips();

        } else {
            enemy_arrivals_[turns_remaining] += fleet->NumShips();
        }
    }
    
    //Update current planet states.
	const int current_owner = planet_->Owner();
	const int growth_rate = planet_->GrowthRate();
	const int num_ships = planet_->NumShips();
    owner_[0] = current_owner;
    ships_[0] = num_ships;
    ships_to_take_over_[0] = 0;
	ships_free_[0] = (current_owner == kMe ? num_ships : 0);
	enemy_ships_free_[0] = (current_owner == kEnemy ? num_ships : 0);

    departing_actions_.clear();

    ships_gained_[0] = planet_->GrowthRate() * OwnerMultiplier(current_owner);
    will_not_be_mine_ = (kMe != current_owner);
    will_be_mine_ = (kMe == current_owner);
    will_not_be_enemys_ = (kEnemy != current_owner);
    will_be_enemys_ = (kEnemy == current_owner);

    this->RecalculateTimeline(1);
}

int PlanetTimeline::ShipsGainedForActions(const ActionList& actions) const {
    if (actions.empty()) {
        return 0;
    }

    //Assume all actions come from the same owner.
    const int action_owner = actions[0]->Owner();
    const int action_owner_multiplier = OwnerMultiplier(action_owner);
    const int my_multiplier = (kMe == action_owner ? 1 : 0);
    const int enemy_multiplier = (kEnemy == action_owner ? 1 : 0);
    const int growth_rate = planet_->GrowthRate();
    
    //Reset additional arrivals.
    for (int i = 0; i < horizon_; ++i) {
        additional_arrivals_[i] = 0;
    }

    //Add the new arrivals to the timeline.
    const uint u_horizon = static_cast<int>(horizon_);
    int start_change = horizon_;

    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        const int distance = action->Distance();
		const int departure_time = action->DepartureTime();
		const int arrival_time = departure_time + distance;
        additional_arrivals_[arrival_time] += action->NumShips();
        start_change = std::min(start_change, arrival_time);
    }

    //Replay the time forward to see how many ships will be gained.
    int ships_gained = 0;
    int ships_on_planet = 0;
    int owner = 0;

    for (int i = 0; i < horizon_; ++i) {
        if (i < start_change) {
            ships_gained += ships_gained_[i];
            owner = owner_[i];
            ships_on_planet = ships_[i];
        
        } else {
            const bool has_arrivals = my_arrivals_[i] != 0
                                    || enemy_arrivals_[i] != 0
                                    || additional_arrivals_[i] != 0;

            if (!has_arrivals) {
                ships_gained += PlanetShipsGainRate(owner, growth_rate);
            
            } else {
                //There do be battles to fight!
                ships_gained += PlanetShipsGainRate(owner, growth_rate);

                const int base_ships = ships_on_planet + (kNeutral == owner ? 0 : growth_rate);
                const int neutral_ships = (kNeutral == owner ? base_ships : 0);
                const int my_ships = my_arrivals_[i] 
                                + additional_arrivals_[i] * my_multiplier
                                + (kMe == owner ? base_ships : 0);
                const int enemy_ships = enemy_arrivals_[i] 
                                    + additional_arrivals_[i] * enemy_multiplier
                                    + (kEnemy == owner ? base_ships : 0);
                
                BattleOutcome outcome = ResolveBattle(owner, neutral_ships, my_ships, enemy_ships);
                ships_on_planet = outcome.ships_remaining;
                owner = outcome.owner;
            }
        }
    }

    ships_gained += growth_rate * kAdditionalGrowthTurns * OwnerMultiplier(owner);
    
    const int additional_ships_gained = ships_gained - total_ships_gained_;
    const int ships_gained_by_action_owner = additional_ships_gained * action_owner_multiplier;

    return ships_gained_by_action_owner;
}

int PlanetTimeline::ShipsRequredToPosess(int arrival_time, int by_whom) const {
    int ships_required = 0;

    if (kMe == by_whom) {
        ships_required = ships_to_take_over_[arrival_time];
    } else {
        ships_required = enemy_ships_to_take_over_[arrival_time];
    }
    
    return ships_required;
}

int PlanetTimeline::ShipsFree(int when, int owner) const {
    int ships_free = 0;

    if (kMe == owner) {
        ships_free = ships_free_[when];
    } else {
        ships_free = enemy_ships_free_[when];
    }

#ifndef IS_SUBMISSION
    if (ships_free > 0) {
        pw_assert(owner == owner_[when]);
    }
#endif

    return ships_free;
}

bool PlanetTimeline::IsOwnedBy(int owner, int when) const {
    const bool is_owned = (owner_[when] == owner);
    return is_owned;
}

bool PlanetTimeline::WillBeOwnedBy(int owner) const {
	const bool will_be_owned = (kMe == owner ? will_be_mine_ : will_be_enemys_);
	return will_be_owned;
}

bool PlanetTimeline::WillNotBeOwnedBy(int owner) const {
	const bool will_not_be_owned = (kMe == owner ? will_not_be_mine_ : will_not_be_enemys_);
	return will_not_be_owned;
}

void PlanetTimeline::AddDeparture(Action *action) {
    const int num_ships = action->NumShips();
    const int departure_time = action->DepartureTime();
    const int growth_rate = planet_->GrowthRate();
    const int action_owner = action->Owner();
    
#ifndef IS_SUBMISSION
    for (uint i = 0; i < departing_actions_.size(); ++i) {
        pw_assert(action != departing_actions_[i] && "Adding a duplicate departure.");
    }
#endif

    departing_actions_.push_back(action);

    std::vector<int>& departures = (action_owner == kMe ? my_departures_ : enemy_departures_);
    std::vector<int>& ships_free = (action_owner == kMe ? ships_free_ : enemy_ships_free_);
    std::vector<int>& contingent_departures = (action_owner == kMe ? my_contingent_departures_ : enemy_contingent_departures_);

#ifndef IS_SUBMISSION
    if (21 == id_ && 28 == num_ships && departure_time == 28) {
        int sj = 2;
    }
#endif

    //In case of contingent departures (i.e. departures that only happen in response to certain
    //opponent actions), reserve ships but don't subtract them from the planet.
    if (action->IsContingent()) {
        contingent_departures[departure_time] += num_ships;
        this->ReserveShips(action_owner, departure_time, num_ships);
        return;
    }
    
    //Make sure that the right number of ships exists on the planet.
    pw_assert(num_ships <= ships_[departure_time]);

    //Record the departure.
    departures[departure_time] += num_ships;
    this->ResetStartingData();
    this->RecalculateTimeline(1);
}

void PlanetTimeline::AddArrivals(const ActionList& actions) {
    if (actions.empty()) {
        return;
    }
    
    //Add arrivals to the planet's timeline.
    const int action_owner = actions[0]->Owner();
    std::vector<int>& arrivals = (kMe == action_owner ? my_arrivals_ : enemy_arrivals_);
    int earliest_arrival = horizon_;
    
    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        pw_assert(action_owner == action->Owner());

        const int arrival_time = action->DepartureTime() + action->Distance();
        
        pw_assert(arrival_time < horizon_ && "In PlanetTimeline::AddArrivals");
        arrivals[arrival_time] += action->NumShips();

#ifndef ENEMY_RESERVES_SHIPS_AGAINST_ARRIVALS
        if (kMe == action_owner) {
            my_unreserved_arrivals_[arrival_time] += action->NumShips();
        }
#endif
    
        if (earliest_arrival > arrival_time) {
            earliest_arrival = arrival_time;
        }
    }

    if (!is_recalculating_) {
        this->ResetStartingData();
        this->RecalculateTimeline(1);
    }
}

void PlanetTimeline::RemoveArrival(Action *action) {
    const int departure_time = action->DepartureTime();
    const int distance = action->Distance();
    const int arrival_time = departure_time + distance;
    const int num_ships = action->NumShips();

    std::vector<int>& arrivals = (kMe == action->Owner() ? my_arrivals_ : enemy_arrivals_);
    pw_assert(num_ships <= arrivals[arrival_time]);
    arrivals[arrival_time] -= num_ships;

    if (kMe == action->Owner()) {
        my_unreserved_arrivals_[arrival_time] -= num_ships;
    }
    
    this->ResetStartingData();
    this->RecalculateTimeline(1);

    this->MarkAsChanged();
}

void PlanetTimeline::RemoveDeparture(Action *action) {
    const int departure_time = action->DepartureTime();
    const int num_ships = action->NumShips();

    std::vector<int>& departures = (kMe == action->Owner() ? my_departures_ : enemy_departures_);
    pw_assert(num_ships <= departures[departure_time]);
    departures[departure_time] -= num_ships;

    if (!is_recalculating_) {
        this->ResetStartingData();
        this->RecalculateTimeline(1);
    }

    this->MarkAsChanged();
}

void PlanetTimeline::ResetStartingData() {
    const int growth_rate = 0;

    //Reset the reservation of ships and ship growths.
    for (int i = 0; i < horizon_; ++i) {
        ships_reserved_[i] = 0;
        enemy_ships_reserved_[i] = 0;
        available_growth_[i] = growth_rate;
        enemy_available_growth_[i] = growth_rate;
    }

    //Reset ownership data.
	const int current_owner = planet_->Owner();
    owner_[0] = current_owner;

    ships_gained_[0] = planet_->GrowthRate() * OwnerMultiplier(current_owner);
    will_not_be_mine_ = (kMe != current_owner);
    will_be_mine_ = (kMe == current_owner);
    will_not_be_enemys_ = (kEnemy != current_owner);
    will_be_enemys_ = (kEnemy == current_owner);

    //Reset the ships on surface at start.
    const int ships = planet_->NumShips();
    const int departing_ships = my_departures_[0] + enemy_departures_[0];
    const int ships_on_surface = ships - departing_ships;
    ships_[0] = ships_on_surface;
    ships_to_take_over_[0] = 0;
    
    ships_free_[0] = (kMe == current_owner ? ships_on_surface : 0);
    enemy_ships_free_[0] = (kEnemy == current_owner ? ships_on_surface : 0);

}

void PlanetTimeline::RecalculateTimeline(int starting_at, const bool use_balances) {
    is_recalculating_ = true;
    const int growth_rate = planet_->GrowthRate();
    has_balance_changed_fate_ = false;
    
#ifndef IS_SUBMISSION
    if (22 == id_ && 0 != my_departures_[17] && 0 == enemy_departures_[17]) {
        int hj = 2;
    }
#endif

    for (int i = starting_at; i < horizon_; ++i) {
        const int prev_index = i - 1;
        const int prev_owner = owner_[prev_index];
        const int prev_ships = ships_[prev_index];

        const int base_ships = prev_ships + (kNeutral == prev_owner ? 0 : growth_rate);

        const int neutral_ships = (kNeutral == prev_owner ? base_ships : 0);
        const int my_ships = my_arrivals_[i] + (kMe == prev_owner ? base_ships : 0);
        const int enemy_ships = enemy_arrivals_[i] + (kEnemy == prev_owner ? base_ships : 0);

        //Check whether there will be a battle on the planet in this turn.
        if (0 == my_arrivals_[i] && 0 == enemy_arrivals_[i]) {
            //Ships don't arrive.
            owner_[i] = prev_owner;
            ships_[i] = base_ships;
            pw_assert(ships_[i] >= 0 && "The number of ships must be non-negative");

        } else {
            //A fleet has arrived.  Resolve the battle.
            BattleOutcome outcome = ResolveBattle(prev_owner, neutral_ships, my_ships, enemy_ships);
            const int new_owner = outcome.owner;
            ships_[i] = outcome.ships_remaining;
            owner_[i] = new_owner;

            //Figure out how many ships were necessary to keep the planet.
            if (kMe == prev_owner && enemy_ships > 0) {
                this->ReserveShips(kMe, i, enemy_ships);
            
            } else if (kEnemy == prev_owner && my_ships > 0) {
                const int ships_to_reserve = my_ships - my_unreserved_arrivals_[i];

                if (0 < ships_to_reserve) {
                    this->ReserveShips(kEnemy, i, ships_to_reserve);
                }
            }

            //Account for the opponent possibly sending reinforecements.
            if (use_balances && min_balances_[i] == 0 && prev_owner == kEnemy && kMe == outcome.owner) {
                //Assume that the enemy managed to hold off the attack.
                ships_[i] = 0;
                owner_[i] = kEnemy;
                has_balance_changed_fate_ = true;
            }
        }
        
        //Account for departures.
        const int cur_owner = owner_[i];
        const int ships_before_departures = ships_[i];
        
        if (ships_before_departures < my_departures_[i] || (kMe != cur_owner && my_departures_[i] > 0)) {
            this->RemoveDepartingActions(i, kMe);
        
        } else if (ships_before_departures < enemy_departures_[i] || (kEnemy != cur_owner && enemy_departures_[i] > 0)) {
            this->RemoveDepartingActions(i, kEnemy);

        } else{
            if (my_departures_[i] > 0) {
                this->ReserveShips(kMe, i, my_departures_[i]);
                ships_[i] = ships_before_departures - my_departures_[i];
                pw_assert(0 == enemy_departures_[i] && "Enemy cannot depart my planet");

            } else if (enemy_departures_[i] > 0) {
                this->ReserveShips(kEnemy, i, enemy_departures_[i]);
                ships_[i] = ships_before_departures - enemy_departures_[i];
            }
        }

        //Update the ownership summaries, additional ship data.
        const int cur_ships = ships_[i];
        const int owner_multiplier = OwnerMultiplier(cur_owner);
        const int my_multiplier = (kMe == cur_owner ? 1 : 0);
        const int enemy_multiplier = (kEnemy == cur_owner ? 1 : 0);
        
        will_be_mine_ |= (kMe == cur_owner);
        will_not_be_mine_ |= (kMe != cur_owner);
        will_be_enemys_ |= (kEnemy == cur_owner);
        will_not_be_enemys_ |= (kEnemy != cur_owner);

        ships_free_[i] = std::max(ships_[i] * my_multiplier - ships_reserved_[i], 0);
        enemy_ships_free_[i] = std::max(ships_[i] * enemy_multiplier - ships_reserved_[i], 0);
        
        const int my_total_opponents = std::max(neutral_ships, enemy_ships) - my_ships + (kMe == prev_owner ? 0 : 1);
        const int enemy_total_opponents = std::max(neutral_ships, my_ships) - enemy_ships + (kEnemy == prev_owner ? 0 : 1);
        ships_to_take_over_[i] = my_total_opponents * (1 - my_multiplier);
        enemy_ships_to_take_over_[i] = enemy_total_opponents * (1 - enemy_multiplier);

        ships_gained_[i] = PlanetShipsGainRate(cur_owner, growth_rate);
        available_growth_[i] = (cur_owner == kMe ? growth_rate : 0);
        enemy_available_growth_[i] = (cur_owner == kEnemy ? growth_rate : 0);

        //Reserve ships for contingent departures.
        if (0 < my_contingent_departures_[i]) {
            this->ReserveShips(kMe, i, my_contingent_departures_[i]);
        }

        if (0 < enemy_contingent_departures_[i]) {
            this->ReserveShips(kEnemy, i, enemy_contingent_departures_[i]);
        }

#ifndef IS_SUBMISSION
        if (ships_free_[i] > 0) {
            pw_assert(cur_owner == kMe);
        } 
        if (enemy_ships_free_[i] > 0) {
            pw_assert(cur_owner == kEnemy);
        }

        if (0 == my_arrivals_[i] && 0 == enemy_arrivals_[i]) {
            if (kNeutral != owner_[i]) {
                pw_assert(ships_[i] == ships_[prev_index] + growth_rate - my_departures_[i] - enemy_departures_[i]);

            } else {
                pw_assert(ships_[i] == ships_[prev_index]);
            }
        }
#endif

    } //End iterating over turns.

    //Finish calculations of number of ships at each move necessary to make
    //produce an increase in the number of ships gained over the horizon.
    //At any point when the planet is under my control, check whether it could
    //use some reinforcing ships to fight a battle coming up a few turns later.
    int prev_ships_to_take_over = 0;
    int prev_enemy_ships_to_take_over = 0;
    for (int i = horizon_ - 1; i >= 0; --i) {
        if (i == horizon_ - 1) {
            prev_ships_to_take_over = ships_to_take_over_[i];
            prev_enemy_ships_to_take_over = enemy_ships_to_take_over_[i];
            //No changes need to be made the the ships gained.

        } else {
            if (0 == ships_to_take_over_[i]) {
                ships_to_take_over_[i] = prev_ships_to_take_over;
            
            } else {
                prev_ships_to_take_over = ships_to_take_over_[i];
            }

            if (0 == enemy_ships_to_take_over_[i]) {
                enemy_ships_to_take_over_[i] = prev_enemy_ships_to_take_over;
            } else {
                prev_enemy_ships_to_take_over = enemy_ships_to_take_over_[i];
            }
        }
    }

    //Tally up the total number of ships gained over the forecast horizon.
    total_ships_gained_ = 0;

    for (unsigned int i = 0; i < ships_gained_.size(); ++i) {
        total_ships_gained_ += ships_gained_[i];
    }

    total_ships_gained_ += growth_rate * kAdditionalGrowthTurns * OwnerMultiplier(owner_[horizon_ - 1]);

    is_recalculating_ = false;
}

void PlanetTimeline::ReserveShips(const int owner, const int key_time, const int num_ships) {
    pw_assert(key_time < horizon_);
    
#ifndef IS_SUBMISSION
    if (1 == id_ && 6 == 0 && 1 == num_ships && 14 == key_time) {
        int sd = 2;
    }
#endif

    std::vector<int>& ships_reserved = (owner == kMe ? ships_reserved_ : enemy_ships_reserved_);
    std::vector<int>& ships_free = (owner == kMe ? ships_free_ : enemy_ships_free_);
    std::vector<int>& available_growth = (owner == kMe ? available_growth_ : enemy_available_growth_);
    
    const int growth_rate = planet_->GrowthRate();
    int ships_to_reserve = num_ships;
    int next_ships_to_reserve = ships_reserved[key_time];

    for (int i = key_time - 1; i >= 0; --i) {
        //Account for possible growth of ships.
        const int growth_towards_reservations = std::min(available_growth[i], ships_to_reserve);
        available_growth[i] -= growth_towards_reservations;
        ships_to_reserve -= growth_towards_reservations;

        //Reserve the ships, if necessary.
        if (0 >= ships_to_reserve) {
            break;

        } else {
            ships_reserved[i] += ships_to_reserve;
            const int ships_unavailable = ships_reserved[i];
            const int owned_ships_on_planet = (owner == owner_[i] ? ships_[i] : 0);
            ships_free[i] = std::max(owned_ships_on_planet - ships_unavailable, 0);

        }
    }
}

void PlanetTimeline::RecalculateShipsGained() {
    //Recalculate ships gained after a balance calculation.
    bool would_planet_be_lost = false;
    const int growth_rate = planet_->GrowthRate();
    total_ships_gained_ = ships_gained_[0];
    int balance_loss = 0;

    for (int t = 1; t < horizon_; ++t) {
        if (would_planet_be_lost && this->OwnerAt(t) == kMe) {
            total_ships_gained_ -= growth_rate;
            balance_loss -= growth_rate * 2;
        
        } else {
            total_ships_gained_ += ships_gained_[t];
        }

        if (owner_[t] == kMe && (min_balances_[t] < 0 || (0 == min_balances_[t] && kEnemy == owner_[t-1]))) {
            would_planet_be_lost = true;
        
        } else if (would_planet_be_lost && (min_balances_[t] + balance_loss > 0)) {
            would_planet_be_lost = false;
        }
    }

    if (would_planet_be_lost) {
        total_ships_gained_ -= kAdditionalGrowthTurns * growth_rate;
    
    } else {
        total_ships_gained_ += OwnerMultiplier(owner_[horizon_ - 1]) * kAdditionalGrowthTurns * growth_rate;
    }
}

void PlanetTimeline::SetReinforcer(bool is_reinforcer) {
    is_reinforcer_ = is_reinforcer;
    this->MarkAsChanged();
}

int PlanetTimeline::NeutralBalanceAdjustment(const int when) const {
    const int planet_owner = this->OwnerAt(when);
    const int was_neutral = (kNeutral == this->OwnerAt(when - 1));
    const int prev_ships = this->ShipsAt(when - 1);
    const int my_arrivals = this->MyArrivalsAt(when);
    const int enemy_arrivals = this->EnemyArrivalsAt(when);
    const int my_neutral_shortage = std::max(prev_ships - my_arrivals, 0);
    const int enemy_neutral_shortage = std::max(prev_ships - enemy_arrivals, 0);

    const int my_neutral_adjustment = (kEnemy == planet_owner && was_neutral) ? my_neutral_shortage : 0;
    const int enemy_neutral_adjustment = (kMe == planet_owner && was_neutral) ? enemy_neutral_shortage : 0;
    const int neutral_adjustment = enemy_neutral_adjustment - my_neutral_adjustment;

    return neutral_adjustment;
}

void PlanetTimeline::RemoveDepartingAction(const uint departure_index) {
    Action* departure_to_remove = departing_actions_[departure_index];
    
    const uint last_departure = departing_actions_.size() - 1;

    for (uint i = departure_index; i < last_departure; ++i) {
        departing_actions_[i] = departing_actions_[i + 1];
    }

    departing_actions_.resize(last_departure);

    this->RemoveDeparture(departure_to_remove);
    departure_to_remove->Target()->RemoveArrival(departure_to_remove);
}

void PlanetTimeline::RemoveDepartingActions(const int turn, const int player) {
    uint i = 0;

    while (i < departing_actions_.size()) {
        Action* action = departing_actions_[i];

        if (action->DepartureTime() == turn && action->Owner() == player && !action->IsContingent()) {
            this->RemoveDepartingAction(i);

        } else {
            ++i;
        }
    }
}

void PlanetTimeline::MarkAsChanged() {
    game_timeline_->MarkTimelineAsModified(id_);
}
