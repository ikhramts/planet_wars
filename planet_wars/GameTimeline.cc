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

//int GameTimeline::ShipsGainedForFleets(const FleetList &fleets, Planet *planet) const {
//    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
//    const int ships_gained = timeline->ShipsGainedForFleets(fleets);
//    return ships_gained;
//}

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

PlanetTimelineList GameTimeline::ForecastersThatWillNotBeMine() const {
    PlanetTimelineList will_not_be_mine;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->WillNotBeMine()) {
            will_not_be_mine.push_back(timeline);
        }
    }

    return will_not_be_mine;
}

int GameTimeline::ShipsRequredToPosess(Planet* planet, int arrival_time) const {
    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
    const int ships_needed = timeline->ShipsRequredToPosess(arrival_time);
    return ships_needed;
}

int GameTimeline::ShipsRequiredToKeep(Planet* planet, int arrival_time) const {
    PlanetTimeline* timeline = planet_timelines_[planet->Id()];
    const int ships_needed = timeline->ShipsRequiredToKeep(arrival_time);
    return ships_needed;
}

PlanetTimelineList GameTimeline::ForecastersOwnedBy(int owner, int when) const {
    PlanetTimelineList forecasters;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (timeline->IsOwnedBy(owner, when)) {
            forecasters.push_back(timeline);
        }
    }

    return forecasters;
}

PlanetTimelineList GameTimeline::ForecastersNotOwnedBy(int owner, int when) const {
    PlanetTimelineList forecasters;

    for (uint i = 0; i < planet_timelines_.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[i];

        if (!timeline->IsOwnedBy(owner, when)) {
            forecasters.push_back(timeline);
        }
    }

    return forecasters;
}

PlanetTimelineList GameTimeline::OwnedForecastersByDistance(int owner, 
                                                                PlanetTimeline* source, 
                                                                int when) {
    PlanetList planets = game_->PlanetsByDistance(source->Id());
    PlanetTimelineList forecasters;
    
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetTimeline* timeline = planet_timelines_[planets[i]->Id()];

        if (timeline->IsOwnedBy(owner, when)) {
            forecasters.push_back(timeline);
        }
    }
    
    return forecasters;
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

    id_ = planet->Id();
    start_ = 0;
    end_ = u_horizon - 1;
    owner_.resize(u_horizon, 0);
    ships_.resize(u_horizon, 0);
    my_arrivals_.resize(u_horizon, 0);
    enemy_arrivals_.resize(u_horizon, 0);
    ships_to_take_over_.resize(u_horizon, 0);
    ships_gained_.resize(u_horizon, 0);
    ships_to_keep_.resize(u_horizon, 0);
    ships_reserved_.resize(u_horizon, 0);
    ships_free_.resize(u_horizon, 0);
    
    will_not_be_mine_ = false;
    
    my_additional_arrivals_at_.resize(u_horizon, 0);
    enemy_additional_arrivals_at_.resize(u_horizon, 0);

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
    
    owner_[0] = planet->Owner();
    ships_[0] = planet->NumShips();
    
    switch (owner_[0]) {
        case kNeutral: ships_gained_[0] = 0; break;
        case kMe: ships_gained_[0] = planet->GrowthRate(); break;
        case kEnemy: ships_gained_[0] = planet->GrowthRate(); break;
        default: pw_assert(!"Invalid owner");
    }

    this->CalculatePlanetStateProjections(1);
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
    for (uint i = 0; i < my_additional_arrivals_at_.size(); ++i) {
        my_additional_arrivals_at_[i] = 0;
        ships_reserved_[i] = 0;
        ships_free_[i] = 0;
    }

    //Update the fleet arrivals.
    FleetList arrivingFleets = game_->FleetsArrivingAt(planet_);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];
        const int turns_remaining = fleet->TurnsRemaining();
        
        //Add only fleets that have just departed.
        if ((turns_remaining + 1) == fleet->TripLength()) {
            start_update_at = std::min(turns_remaining, start_update_at);

            if (fleet->Owner() == kMe) {
                my_arrivals_[turns_remaining] += fleet->NumShips();

            } else {
                enemy_arrivals_[turns_remaining] += fleet->NumShips();
            }
        }
    }
    
    //Update current planet states.
    owner_[start_] = planet_->Owner();
    ships_[start_] = planet_->NumShips();
    ships_to_take_over_[start_] = 0;

    this->CalculatePlanetStateProjections(1);
}

int PlanetTimeline::ShipsGainedForActions(const ActionList& actions) const {
    const int growth_rate = planet_->GrowthRate();
    
    const uint u_horizon = static_cast<int>(horizon_);
    int start_change = horizon_;

    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        const int turns_remaining = action->Distance();
        my_additional_arrivals_at_[turns_remaining] += action->NumShips();
        start_change = std::min(start_change, turns_remaining);
    }

    //Replay the time forward to see how many ships will be gained.
    bool path_diverged = false;
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
                                    || my_additional_arrivals_at_[i] != 0;

            if (!has_arrivals) {
                ships_gained += PlanetShipsGainRate(owner, growth_rate);
            
            } else {
                //There do be battles to fight!
                ships_gained += PlanetShipsGainRate(owner, growth_rate);

                const int base_ships = ships_on_planet + (kNeutral == owner ? 0 : growth_rate);
                const int neutral_ships = (kNeutral == owner ? base_ships : 0);
                const int my_ships = my_arrivals_[cur_index] 
                                + my_additional_arrivals_at_[i]
                                + (kMe == owner ? base_ships : 0);
                const int enemy_ships = my_arrivals_[cur_index] 
                                    + (kEnemy == owner ? base_ships : 0);
                
                BattleOutcome outcome = ResolveBattle(owner, neutral_ships, my_ships, enemy_ships);
                ships_on_planet = outcome.ships_remaining;
                owner = outcome.owner;
            }
        }
    }
    
    const int additional_ships_gained = ships_gained - total_ships_gained_;
    return additional_ships_gained;
}

int PlanetTimeline::ShipsRequredToPosess(int arrival_time) const {
    const int actual_index = this->ActualIndex(arrival_time);
    const int ships_required = ships_to_take_over_[actual_index];
    return ships_required;
}

int PlanetTimeline::ShipsRequiredToKeep(int arrival_time) const {
    const int actual_index = this->ActualIndex(arrival_time);
    const int ships_required = ships_to_keep_[actual_index];
    return ships_required;
}

bool PlanetTimeline::IsOwnedBy(int owner, int when) const {
    const int actual_index = this->ActualIndex(when);
    const bool is_owned = (owner_[actual_index] == owner);
    return is_owned;
}

void PlanetTimeline::AddDeparture(Action *action) {
    const int num_ships = action->NumShips();
    const int departure_time = action->DepartureTime();
    const int growth_rate = planet_->GrowthRate();
    const int action_owner = action->Owner();

    //Make sure that the right number of ships exists on the planet.
    pw_assert(num_ships >= ships_[this->ActualIndex(departure_time)]);
    
    //Reserve the ships prior to departure
    int ships_to_reserve = num_ships;

    for (int i = departure_time - 1; i >= 0; --i) {
        const int cur_index = this->ActualIndex(i);

        //Account for possible growth of ships.
        if (action_owner == owner_[cur_index]) {
            ships_to_reserve -= growth_rate;
        }
        
        //Reserve the ships, if necessary.
        if (0 >= ships_to_reserve) {
            break;

        } else {
            ships_reserved_[cur_index] += ships_to_reserve;
            const int ships_unavailable = std::max(ships_reserved_[cur_index], ships_to_keep_[cur_index]);
            const int my_ships_on_planet = (kMe == owner_[i] ? ships_[cur_index] : 0);
            ships_free_[cur_index] = std::max(my_ships_on_planet - ships_unavailable, 0);
        }
    }

    //Subtract the ships after departure.
    for (int i = departure_time; i < horizon_; ++i) {
        const int cur_index = this->ActualIndex(i);
        const int ships_remaining = ships_[cur_index] - num_ships;

        if (0 > ships_remaining) {
            //For now, the calculations must cause ownership changes.
            pw_assert("A departure caused ownership change in PlanetTimeline::AddDeparture.");
            
            //Ownership has changed; must recalculate the timeline from this point.
            //this->CalculatePlanetStateProjections(i);
            break;

        } else {
            ships_[cur_index] = ships_remaining;
        }
    }
}

void PlanetTimeline::AddArrivals(const ActionList& actions) {
    //Add arrivals to the planet's timeline.
    int earliest_arrival = horizon_;
    
    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        const int arrival_time = action->DepartureTime() + action->Distance();
        const int arrival_index = this->ActualIndex(arrival_time);
        
        pw_assert(arrival_time < horizon_ && "In PlanetTimeline::AddArrivals");
        
        if (kMe == action->Owner()) {
            my_arrivals_[arrival_index] += action->NumShips();

        } else {
            enemy_arrivals_[arrival_index] += action->NumShips();
        }

        if (earliest_arrival > arrival_time) {
            earliest_arrival = arrival_time;
        }
    }

    this->CalculatePlanetStateProjections(earliest_arrival);
}

void PlanetTimeline::CalculatePlanetStateProjections(int starting_at) {
    const int growth_rate = planet_->GrowthRate();
    
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
            ships_[cur_index] = outcome.ships_remaining;
            owner_[cur_index] = outcome.owner;
        }

        //Check how many ships are necessary at this point to make the planet mine.
        const int cur_owner = owner_[cur_index];
        const int cur_ships = ships_[cur_index];

        if (kMe == cur_owner) {
            ships_to_take_over_[cur_index] = 0;

        } else if (kNeutral == cur_owner) {
            ships_to_take_over_[cur_index] = prev_ships - my_arrivals_[cur_index] + 1;

        } else {
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
    } //End iterating over turns.

    //Finish calculations of number of ships at each move necessary to make
    //produce an increase in the number of ships gained over the horizon.
    //At any point when the planet is under my control, check whether it could
    //use some reinforcing ships to fight a battle coming up a few turns later.
    //While we're at it, check whether the planet will not be mine at any point.
    int prev_ships_to_take_over = 0;
    will_not_be_mine_ = false;

    for (int i = horizon_ - 1; i >= 0; --i) {
        const int cur_index = this->ActualIndex(i);

        if (i == horizon_ - 1) {
            prev_ships_to_take_over = ships_to_take_over_[cur_index];
            //No changes need to be made the the ships gained.

        } else {
            if (0 == ships_to_take_over_[cur_index]) {
                ships_to_take_over_[cur_index] = prev_ships_to_take_over;
            
            } else {
                prev_ships_to_take_over = ships_to_take_over_[cur_index];
                will_not_be_mine_ = true;
            }
        }
    }

    //Tally up the total number of ships gained over the forecast horizon.
    total_ships_gained_ = 0;

    for (unsigned int i = 0; i < ships_gained_.size(); ++i) {
        total_ships_gained_ += ships_gained_[i];
    }

    //Calculate the minimum number of ships required to keep posession of the
    //planet.  To do that, run backwards in time through all upcoming fleets.
    int prev_ships_to_keep = 0;
    for (int i = horizon_ - 1; i >= 0; --i) {
        int cur_index = this->ActualIndex(i);
        const int ships_for_next_turn = prev_ships_to_keep - growth_rate;
        const int turn_arrival_balance = enemy_arrivals_[cur_index] - my_arrivals_[cur_index];
        const int ships_to_keep = std::max(ships_for_next_turn + turn_arrival_balance, 0);

        ships_to_keep_[cur_index] = ships_to_keep;
        prev_ships_to_keep = ships_to_keep;

        //Update the number of reserved ships.
        const int my_ships = (kMe == owner_[cur_index] ? ships_[cur_index] : 0);
        const int ships_unavailable = std::max(ships_to_keep, ships_reserved_[cur_index]);
        const int ships_free = std::max(my_ships - ships_unavailable, 0);
        ships_free_[cur_index] = ships_free;
    }
}
