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
    }
}

void GameTimeline::SetGameMap(GameMap* game) {
    game_ = game;
    horizon_ = game->MapRadius() + 5;

    //Update the ships gained from planets vector.
    PlanetList planets = game->Planets();
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = new PlanetTimeline();
        timeline->Initialize(horizon_, planets[i], game);
        planet_timelines_.push_back(timeline);
    }
}

void GameTimeline::Update() {
    //Update the planet data.
    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        planet_timelines_[i]->Update();
    }
}

int GameTimeline::ShipsGainedForActions(const ActionList& actions, Planet *planet) const {
    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
    const int ships_gained = timeline->ShipsGainedForActions(actions);
    return ships_gained;
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

void GameTimeline::ApplyActions(const ActionList& actions) {
	if (actions.empty()) {
		return;
	}
	
	//Assumption: actions are grouped by their target.
	
	//Apply the actions to their target.
	PlanetTimeline* target = actions[0]->Target();
	target->AddArrivals(actions);

	//Apply the actions to their sources.
	for (uint i = 0; i < actions.size(); ++i) {
		pw_assert(target->Id() == actions[i]->Target()->Id() && "Applied actions must have same target");

		PlanetTimeline* source = actions[i]->Source();
		source->AddDeparture(actions[i]);
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

/************************************************
               PlanetTimeline class
************************************************/
PlanetTimeline::PlanetTimeline()
:game_(NULL), planet_(NULL) {
}

void PlanetTimeline::Initialize(int forecast_horizon, Planet *planet, GameMap *game) {
    horizon_ = forecast_horizon;
    planet_ = planet;
    game_ = game;

    //Initialize the forecast arrays.
    const size_t u_horizon = static_cast<size_t>(horizon_);
    const int growth_rate = planet->GrowthRate();

    id_ = planet->Id();
    start_ = 0;
    end_ = u_horizon - 1;
    owner_.resize(u_horizon, 0);
    ships_.resize(u_horizon, 0);
    my_arrivals_.resize(u_horizon, 0);
    enemy_arrivals_.resize(u_horizon, 0);
    ships_to_take_over_.resize(u_horizon, 0);
    ships_gained_.resize(u_horizon, 0);
    available_growth_.resize(u_horizon, growth_rate);
//    ships_to_keep_.resize(u_horizon, 0);
    ships_reserved_.resize(u_horizon, 0);
    ships_free_.resize(u_horizon, 0);

    enemy_ships_to_take_over_.resize(u_horizon, 0);
//    enemy_ships_to_keep_.resize(u_horizon, 0);
    enemy_ships_reserved_.resize(u_horizon, 0);
    enemy_ships_free_.resize(u_horizon, 0);
    enemy_available_growth_.resize(u_horizon, growth_rate);

    my_departures_.resize(u_horizon, 0);
    enemy_departures_.resize(u_horizon, 0);
    
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

    this->RecalculateTimeline(1);
}

void PlanetTimeline::Update() {
    //Advance the forecast frame through time.
    start_ = (start_ + 1) % horizon_;
	end_ = this->ActualIndex(-1);
    
    my_arrivals_[end_] = 0;
    enemy_arrivals_[end_] = 0;
    
    int start_update_at = horizon_ - 1;

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
    }

    //Update the fleet arrivals.
    FleetList arrivingFleets = game_->FleetsArrivingAt(planet_);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];
        const int turns_remaining = fleet->TurnsRemaining();
        const int arrival_index = this->ActualIndex(turns_remaining);
        
        //Add only fleets that have just departed.
        //if ((turns_remaining + 1) == fleet->TripLength()) {
        start_update_at = std::min(turns_remaining, start_update_at);

        if (fleet->Owner() == kMe) {
            my_arrivals_[arrival_index] += fleet->NumShips();

        } else {
            enemy_arrivals_[arrival_index] += fleet->NumShips();
        }
        //}
    }
    
    //Update current planet states.
	const int current_owner = planet_->Owner();
	const int growth_rate = planet_->GrowthRate();
	const int num_ships = planet_->NumShips();
    owner_[start_] = current_owner;
    ships_[start_] = num_ships;
    ships_to_take_over_[start_] = 0;
	ships_free_[start_] = (current_owner == kMe ? num_ships : 0);
	enemy_ships_free_[start_] = (current_owner == kEnemy ? num_ships : 0);

    ships_gained_[start_] = planet_->GrowthRate() * OwnerMultiplier(current_owner);
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
        const int cur_index = (start_ + i) % horizon_;

        if (i < start_change) {
            ships_gained += ships_gained_[cur_index];
            owner = owner_[cur_index];
            ships_on_planet = ships_[cur_index];
        
        } else {
            const bool has_arrivals = my_arrivals_[cur_index] != 0
                                    || enemy_arrivals_[i] != 0
                                    || additional_arrivals_[i] != 0;

            if (!has_arrivals) {
                ships_gained += PlanetShipsGainRate(owner, growth_rate);
            
            } else {
                //There do be battles to fight!
                ships_gained += PlanetShipsGainRate(owner, growth_rate);

                const int base_ships = ships_on_planet + (kNeutral == owner ? 0 : growth_rate);
                const int neutral_ships = (kNeutral == owner ? base_ships : 0);
                const int my_ships = my_arrivals_[cur_index] 
                                + additional_arrivals_[i] * my_multiplier
                                + (kMe == owner ? base_ships : 0);
                const int enemy_ships = enemy_arrivals_[cur_index] 
                                    + additional_arrivals_[i] * enemy_multiplier
                                    + (kEnemy == owner ? base_ships : 0);
                
                BattleOutcome outcome = ResolveBattle(owner, neutral_ships, my_ships, enemy_ships);
                ships_on_planet = outcome.ships_remaining;
                owner = outcome.owner;
            }
        }
    }
    
    const int additional_ships_gained = ships_gained - total_ships_gained_;
    const int ships_gained_by_action_owner = additional_ships_gained * action_owner_multiplier;
    return ships_gained_by_action_owner;
}

int PlanetTimeline::ShipsRequredToPosess(int arrival_time, int by_whom) const {
    const int actual_index = this->ActualIndex(arrival_time);
    int ships_required = 0;

    if (kMe == by_whom) {
        ships_required = ships_to_take_over_[actual_index];
    } else {
        ships_required = enemy_ships_to_take_over_[actual_index];
    }
    
    return ships_required;
}

int PlanetTimeline::ShipsFree(int when, int owner) const {
    const int actual_index = this->ActualIndex(when);
    int ships_free = 0;

    if (kMe == owner) {
        ships_free = ships_free_[actual_index];
    } else {
        ships_free = enemy_ships_free_[actual_index];
    }

#ifndef IS_SUBMISSION
    if (ships_free > 0) {
        pw_assert(owner == owner_[actual_index]);
    }
#endif

    return ships_free;
}

bool PlanetTimeline::IsOwnedBy(int owner, int when) const {
    const int actual_index = this->ActualIndex(when);
    const bool is_owned = (owner_[actual_index] == owner);
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

    std::vector<int>& departures = (action_owner == kMe ? my_departures_ : enemy_departures_);
    std::vector<int>& ships_free = (action_owner == kMe ? ships_free_ : enemy_ships_free_);

#ifndef IS_SUBMISSION
    if (21 == id_ && 28 == num_ships && departure_time == 28) {
        int sj = 2;
    }
#endif

    //Make sure that the right number of ships exists on the planet.
    pw_assert(num_ships <= ships_[this->ActualIndex(departure_time)]);

    //Record the departure.
    const int departure_index = this->ActualIndex(departure_time);
    departures[departure_index] += num_ships;

    //Calculate effects on the timeline due to the departure.
    this->ReserveShips(action_owner, departure_time, num_ships);

    for (int i = departure_time; i < horizon_; ++i) {
        const int cur_index = this->ActualIndex(i);
        const int ships_remaining = ships_[cur_index] - num_ships;

        if (0 > ships_remaining) {
            //For now, the calculations must cause ownership changes.
            pw_assert("A departure caused ownership change in PlanetTimeline::AddDeparture.");
            
            //Ownership has changed; must recalculate the timeline from this point.
            //this->RecalculateTimeline(i);
            break;

        } else {
            ships_[cur_index] = ships_remaining;
            pw_assert(ships_free[cur_index] >= num_ships && "Must not take more ships than there are free ships");
            ships_free[cur_index] -= num_ships;
        }

#ifndef IS_SUBMISSION
        const int prev_index = this->ActualIndex(i - 1);
        
        //Check that the number of free ships is non-decreasing.
        if (i > 0) {
            pw_assert(ships_free[cur_index] >= ships_free[prev_index]);
        }
        
#endif
    }
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
        const int arrival_index = this->ActualIndex(arrival_time);
        
        pw_assert(arrival_time < horizon_ && "In PlanetTimeline::AddArrivals");
        arrivals[arrival_index] += action->NumShips();
        
        if (earliest_arrival > arrival_time) {
            earliest_arrival = arrival_time;
        }
    }
    
    this->ResetStartingData();
    this->RecalculateTimeline(1);
}

void PlanetTimeline::RemoveArrival(Action *action) {
    const int departure_time = action->DepartureTime();
    const int distance = action->Distance();
    const int arrival_time = departure_time + distance;
    const int arrival_index = this->ActualIndex(arrival_time);
    const int num_ships = action->NumShips();

    std::vector<int>& arrivals = (kMe == action->Owner() ? my_arrivals_ : enemy_arrivals_);
    pw_assert(num_ships <= arrivals[arrival_index]);
    arrivals[arrival_index] -= num_ships;
}

void PlanetTimeline::RemoveDeparture(Action *action) {
    const int departure_time = action->DepartureTime();
    const int departure_index = this->ActualIndex(departure_time);
    const int num_ships = action->NumShips();

    std::vector<int>& departures = (kMe == action->Owner() ? my_departures_ : enemy_departures_);
    pw_assert(num_ships <= departures[departure_index]);
    departures[departure_index] -= num_ships;
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
    owner_[start_] = current_owner;

    ships_gained_[start_] = planet_->GrowthRate() * OwnerMultiplier(current_owner);
    will_not_be_mine_ = (kMe != current_owner);
    will_be_mine_ = (kMe == current_owner);
    will_not_be_enemys_ = (kEnemy != current_owner);
    will_be_enemys_ = (kEnemy == current_owner);

    //Reset the ships on surface at start.
    const int ships = planet_->NumShips();
    const int departing_ships = my_departures_[start_] + enemy_departures_[start_];
    const int ships_on_surface = ships - departing_ships;
    ships_[start_] = ships_on_surface;
    ships_to_take_over_[start_] = 0;
    
    ships_free_[start_] = (kMe == current_owner ? ships_on_surface : 0);
    enemy_ships_free_[start_] = (kEnemy == current_owner ? ships_on_surface : 0);

}

void PlanetTimeline::RecalculateTimeline(int starting_at) {
    const int growth_rate = planet_->GrowthRate();
    
#ifndef IS_SUBMISSION
    if (20 == id_) {
        int hj = 2;
    }
#endif

    for (int i = starting_at; i < horizon_; ++i) {
        const int prev_index = (start_ + i - 1 + horizon_) % horizon_;
        const int cur_index = (start_ + i) % horizon_;
        const int prev_owner = owner_[prev_index];
        const int prev_ships = ships_[prev_index];

        const int base_ships = prev_ships + (kNeutral == prev_owner ? 0 : growth_rate);

        //Check whether any ships arrive at the planet in this turn.
        if (0 == my_arrivals_[cur_index] && 0 == enemy_arrivals_[cur_index]) {
            //Ships don't arrive.
            owner_[cur_index] = prev_owner;
            ships_[cur_index] = base_ships;

        } else {
            //A fleet has arrived.  Resolve the battle.
            const int neutral_ships = (kNeutral == prev_owner ? base_ships : 0);
            const int my_ships = my_arrivals_[cur_index] + (kMe == prev_owner ? base_ships : 0);
            const int enemy_ships = enemy_arrivals_[cur_index] + (kEnemy == prev_owner ? base_ships : 0);
            
            BattleOutcome outcome = ResolveBattle(prev_owner, neutral_ships, my_ships, enemy_ships);
            const int new_owner = outcome.owner;
            ships_[cur_index] = outcome.ships_remaining;
            owner_[cur_index] = new_owner;

            //Figure out how many ships were necessary to keep the planet.
            if (kMe == prev_owner && enemy_ships > 0) {
                this->ReserveShips(kMe, i, enemy_ships);
            
            } else if (kEnemy == prev_owner && my_ships > 0) {
                this->ReserveShips(kEnemy, i, my_ships);
            }
        }

        //Check how many ships are necessary at this point to make the planet mine.
        const int cur_owner = owner_[cur_index];
        const int cur_ships = ships_[cur_index];

        if (kMe == cur_owner) {
			will_be_mine_ = true;
            will_not_be_enemys_ = true;
            ships_to_take_over_[cur_index] = 0;

            enemy_ships_free_[cur_index] = 0;
            ships_free_[cur_index] = std::max(ships_[cur_index] - ships_reserved_[cur_index], 0);

            if (kNeutral == prev_owner) {
                enemy_ships_to_take_over_[cur_index] = 
                    my_arrivals_[cur_index] - enemy_arrivals_[cur_index] + 1;

            } else if (kMe == prev_owner) {
                enemy_ships_to_take_over_[cur_index] = my_arrivals_[cur_index] 
                                                    - enemy_arrivals_[cur_index]
                                                    + base_ships;
            } else { //kEnemy == prev_owner
                enemy_ships_to_take_over_[cur_index] = my_arrivals_[cur_index] 
                                                    - base_ships
                                                    - enemy_arrivals_[cur_index] + 1;
            }

        } else if (kNeutral == cur_owner) {
            ships_to_take_over_[cur_index] = prev_ships - my_arrivals_[cur_index] + 1;
            enemy_ships_to_take_over_[cur_index] = prev_ships - enemy_arrivals_[cur_index] + 1;

        } else {
			will_not_be_mine_ = true;
			will_be_enemys_ = true;
            enemy_ships_to_take_over_[cur_index] = 0;

            ships_free_[cur_index] = 0;
            enemy_ships_free_[cur_index] = std::max(ships_[cur_index] - enemy_ships_reserved_[cur_index], 0);

            if (kNeutral == prev_owner) {
                ships_to_take_over_[cur_index] = 
                    enemy_arrivals_[cur_index] - my_arrivals_[cur_index] + 1;

            } else if (kMe == prev_owner) {
                ships_to_take_over_[cur_index] = enemy_arrivals_[cur_index] 
                                                    - my_arrivals_[cur_index]
                                                    - base_ships;
            } else { //kEnemy == prev_owner
                ships_to_take_over_[cur_index] = enemy_arrivals_[cur_index] 
                                                    + base_ships
                                                    - my_arrivals_[cur_index] + 1;
            }
        }

        ships_gained_[cur_index] = PlanetShipsGainRate(cur_owner, growth_rate);

#ifndef IS_SUBMISSION
        if (ships_free_[cur_index] > 0) {
            pw_assert(cur_owner == kMe);
        } 
        if (enemy_ships_free_[cur_index]) {
            pw_assert(cur_owner == kEnemy);
        }

        if (0 == my_arrivals_[cur_index] && 0 == enemy_arrivals_[cur_index]) {
            if (kNeutral != owner_[cur_index]) {
                pw_assert(ships_[cur_index] == ships_[prev_index] + growth_rate);

            } else {
                pw_assert(ships_[cur_index] == ships_[prev_index]);
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
        const int cur_index = this->ActualIndex(i);

        if (i == horizon_ - 1) {
            prev_ships_to_take_over = ships_to_take_over_[cur_index];
            prev_enemy_ships_to_take_over = enemy_ships_to_take_over_[cur_index];
            //No changes need to be made the the ships gained.

        } else {
            if (0 == ships_to_take_over_[cur_index]) {
                ships_to_take_over_[cur_index] = prev_ships_to_take_over;
                enemy_ships_to_take_over_[cur_index] = prev_enemy_ships_to_take_over;
            
            } else {
                prev_ships_to_take_over = ships_to_take_over_[cur_index];
                prev_enemy_ships_to_take_over = enemy_ships_to_take_over_[cur_index];
            }
        }
    }

    //Tally up the total number of ships gained over the forecast horizon.
    total_ships_gained_ = 0;

    for (unsigned int i = 0; i < ships_gained_.size(); ++i) {
        total_ships_gained_ += ships_gained_[i];
    }

}

void PlanetTimeline::ReserveShips(const int owner, const int key_time, const int num_ships) {
    pw_assert(key_time < horizon_);
    
#ifndef IS_SUBMISSION
    if (1 == id_ && 6 == start_ && 1 == num_ships && 14 == key_time) {
        int sd = 2;
    }
#endif

    std::vector<int>& ships_reserved = (owner == kMe ? ships_reserved_ : enemy_ships_reserved_);
    std::vector<int>& ships_free = (owner == kMe ? ships_free_ : enemy_ships_free_);
    std::vector<int>& available_growth = (owner == kMe ? available_growth_ : enemy_available_growth_);
    
    const int growth_rate = planet_->GrowthRate();
    int ships_to_reserve = num_ships;
    int next_ships_to_reserve = ships_reserved[this->ActualIndex(key_time)];

    for (int i = key_time - 1; i >= 0; --i) {
        const int cur_index = this->ActualIndex(i);

        //Account for possible growth of ships.
        if (owner == owner_[cur_index]) {
            //Reservations cannot decrease per turn by more than the growth rate.
            const int growth_towards_reservations = std::min(available_growth[cur_index], ships_to_reserve);
            available_growth[cur_index] -= growth_towards_reservations;
            //const int reservation_growth_rate_allowance = std::max(growth_rate - next_ships_to_reserve, 0);
            //next_ships_to_reserve = ships_reserved[cur_index];
            //ships_to_reserve = std::max(ships_to_reserve - reservation_growth_rate_allowance, 0);
            ships_to_reserve -= growth_towards_reservations;
        }

        //Reserve the ships, if necessary.
        if (0 >= ships_to_reserve) {
            break;

        } else {
            ships_reserved[cur_index] += ships_to_reserve;
            //const int ships_unavailable = std::max(ships_reserved_[cur_index], ships_to_keep_[cur_index]);
            const int ships_unavailable = ships_reserved[cur_index];
            const int owned_ships_on_planet = (owner == owner_[cur_index] ? ships_[cur_index] : 0);
            ships_free[cur_index] = std::max(owned_ships_on_planet - ships_unavailable, 0);

        }
    }
}

