//Available under GPLv3.
//Author: Iouri Khramtsov.

#include <assert.h>
#include "GameForecaster.h"

/************************************************
               GameForecaster class
************************************************/
GameForecaster::GameForecaster()
: game_(NULL) {
}

GameForecaster::~GameForecaster() {
    for (uint i = 0; i < planet_forecasters_.size(); ++i) {
        delete planet_forecasters_[i];
    }
}

void GameForecaster::SetGameMap(GameMap* game) {
    game_ = game;

    //Update the ships gained from planets vector.
    PlanetList planets = game->Planets();
    for (uint i = 0; i < planets.size(); ++i) {
        PlanetForecaster* forecaster = new PlanetForecaster();
        forecaster->Initialize(kTimeHorizon, planets[i], game);
        planet_forecasters_.push_back(forecaster);
    }
}

void GameForecaster::Update() {
    //Update the planet data.
    for (uint i = 0; i < planet_forecasters_.size(); ++i) {
        planet_forecasters_[i]->Update();
    }
}

int GameForecaster::ShipsGainedForFleets(const FleetList &fleets, Planet *planet) const {
    PlanetForecaster* forecaster = planet_forecasters_[planet->Id()];
    const int ships_gained = forecaster->ShipsGainedForFleets(fleets);
    return ships_gained;
}

PlanetList GameForecaster::PlanetsThatWillNotBeMine() const {
    PlanetList will_not_be_mine;

    for (uint i = 0; i < planet_forecasters_.size(); ++i) {
        PlanetForecaster* forecaster = planet_forecasters_[i];

        if (forecaster->WillNotBeMine()) {
            will_not_be_mine.push_back(forecaster->GetPlanet());
        }
    }

    return will_not_be_mine;
}

int GameForecaster::ShipsRequredToPosess(Planet* planet, int arrival_time) const {
    PlanetForecaster* forecaster = planet_forecasters_[planet->Id()];
    const int ships_needed = forecaster->ShipsRequredToPosess(arrival_time);
    return ships_needed;
}

/************************************************
               PlanetForecaster class
************************************************/
PlanetForecaster::PlanetForecaster()
:game_(NULL), planet_(NULL) {
}

void PlanetForecaster::Initialize(int forecast_horizon, Planet *planet, GameMap *game) {
    horizon_ = forecast_horizon;
    planet_ = planet;
    game_ = game;

    //Initialize the forecast arrays.
    const size_t u_horizon = static_cast<size_t>(horizon_);

    start_ = 0;
    end_ = u_horizon - 1;
    owner_at_.resize(u_horizon, 0);
    ships_at_.resize(u_horizon, 0);
    my_arrivals_at_.resize(u_horizon, 0);
    enemy_arrivals_at_.resize(u_horizon, 0);
    ships_to_take_over_at_.resize(u_horizon, 1);
    ships_gained_at_.resize(u_horizon, 1);
    
    will_not_be_mine_ = false;
    
    my_additional_arrivals_at_.resize(u_horizon, 0);
    enemy_additional_arrivals_at_.resize(u_horizon, 0);

    //Find out when various ships will be arriving.
    FleetList arrivingFleets = game->FleetsArrivingAt(planet);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];

        if (fleet->Owner() == kMe) {
            my_arrivals_at_[fleet->TurnsRemaining()] += fleet->NumShips();

        } else {
            enemy_arrivals_at_[fleet->TurnsRemaining()] += fleet->NumShips();
        }
    }
    
    owner_at_[0] = planet->Owner();
    ships_at_[0] = planet->NumShips();
    
    switch (owner_at_[0]) {
        case kNeutral: ships_gained_at_[0] = 0; break;
        case kMe: ships_gained_at_[0] = planet->GrowthRate(); break;
        case kEnemy: ships_gained_at_[0] = planet->GrowthRate(); break;
        default: assert(!"Invalid owner");
    }

    this->CalculatePlanetStateProjections(1);
}

void PlanetForecaster::Update() {
    //Advance the forecast frame through time.
    start_ = this->ActualIndex(start_ + 1);
    end_ = this->ActualIndex(end_ + 1);;
    
    my_arrivals_at_[end_] = 0;
    enemy_arrivals_at_[end_] = 0;
    
    int start_update_at = horizon_ - 1;

    //Update the fleet arrivals.
    FleetList arrivingFleets = game_->FleetsArrivingAt(planet_);
    
    for (unsigned int i = 0; i < arrivingFleets.size(); ++i) {
        Fleet* fleet = arrivingFleets[i];
        const int turns_remaining = fleet->TurnsRemaining();
        
        //Add only fleets that have just departed.
        if ((turns_remaining + 1) == fleet->TripLength()) {
            start_update_at = std::min(turns_remaining, start_update_at);

            if (fleet->Owner() == kMe) {
                my_arrivals_at_[turns_remaining] += fleet->NumShips();

            } else {
                enemy_arrivals_at_[turns_remaining] += fleet->NumShips();
            }
        }
    }
    
    //Update current planet states.
    owner_at_[start_] = planet_->Owner();
    ships_at_[start_] = planet_->NumShips();

    this->CalculatePlanetStateProjections(1);
}

int PlanetForecaster::ShipsGainedForFleets(const FleetList& fleets) const {
    const int growth_rate = planet_->GrowthRate();
    
    //Reset the arrivals.
    //For now deal only with my arrivals.
    for (uint i = 0; i < my_additional_arrivals_at_.size(); ++i) {
        my_additional_arrivals_at_[i] = 0;
    }

    const uint u_horizon = static_cast<int>(horizon_);
    int start_change = horizon_;

    for (uint i = 0; i < fleets.size(); ++i) {
        Fleet* fleet = fleets[i];
        const int turns_remaining = fleet->TurnsRemaining();
        my_additional_arrivals_at_[turns_remaining] += fleet->NumShips();
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
            ships_gained += ships_gained_at_[cur_index];
            owner = owner_at_[cur_index];
            ships_on_planet = ships_at_[cur_index];
        
        } else {
            const bool has_arrivals = my_arrivals_at_[cur_index] != 0
                                    || enemy_arrivals_at_[i] != 0
                                    || my_additional_arrivals_at_[i] != 0;

            if (!has_arrivals) {
                ships_gained += PlanetShipsGainRate(owner, growth_rate);
            
            } else {
                //There do be battles to fight!
                ships_gained += PlanetShipsGainRate(owner, growth_rate);

                const int base_ships = ships_on_planet + (kNeutral == owner ? 0 : growth_rate);
                const int neutral_ships = (kNeutral == owner ? base_ships : 0);
                const int my_ships = my_arrivals_at_[cur_index] 
                                + my_additional_arrivals_at_[i]
                                + (kMe == owner ? base_ships : 0);
                const int enemy_ships = my_arrivals_at_[cur_index] 
                                    + (kEnemy == owner ? base_ships : 0);
                
                BattleOutcome outcome = ResolveBattle(owner, neutral_ships, my_ships, enemy_ships);
                ships_on_planet = outcome.ships_remaining;
                owner = outcome.owner;
            }
        }
    }
    
    const int additional_ships_gained = ships_gained - ships_gained_;
    return additional_ships_gained;
}

int PlanetForecaster::ShipsRequredToPosess(int arrival_time) const {
    const int actual_index = this->ActualIndex(arrival_time);
    const int ships_required = ships_to_take_over_at_[actual_index];
    return ships_required;
}

void PlanetForecaster::CalculatePlanetStateProjections(int starting_at) {
    const int growth_rate = planet_->GrowthRate();
    
    for (int i = starting_at; i < horizon_; ++i) {
        const int prev_index = (start_ + i - 1 + horizon_) % horizon_;
        const int cur_index = (start_ + i) % horizon_;
        const int prev_owner = owner_at_[prev_index];
        const int prev_ships = ships_at_[prev_index];

        const int base_ships = prev_ships + (kNeutral == prev_owner ? 0 : growth_rate);

        //Check whether any ships arrive at the planet in this turn.
        if (0 == my_arrivals_at_[cur_index] && 0 == enemy_arrivals_at_[cur_index]) {
            //Ships don't arrive.
            owner_at_[cur_index] = prev_owner;
            ships_at_[cur_index] = base_ships;

        } else {
            //A fleet has arrived.  Resolve the battle.
            const int neutral_ships = (kNeutral == prev_owner ? base_ships : 0);
            const int my_ships = my_arrivals_at_[cur_index] + (kMe == prev_owner ? base_ships : 0);
            const int enemy_ships = enemy_arrivals_at_[cur_index] + (kEnemy == prev_owner ? base_ships : 0);
            
            BattleOutcome outcome = ResolveBattle(prev_owner, neutral_ships, my_ships, enemy_ships);
            ships_at_[cur_index] = outcome.ships_remaining;
            owner_at_[cur_index] = outcome.owner;
        }

        //Check how many ships are necessary at this point to make the planet mine.
        const int cur_owner = owner_at_[cur_index];
        const int cur_ships = ships_at_[cur_index];

        if (kMe == cur_owner) {
            ships_to_take_over_at_[cur_index] = 0;

        } else if (kNeutral == cur_owner) {
            ships_to_take_over_at_[cur_index] = prev_ships - my_arrivals_at_[cur_index] + 1;

        } else {
            if (kNeutral == prev_owner) {
                ships_to_take_over_at_[cur_index] = 
                    enemy_arrivals_at_[cur_index] - my_arrivals_at_[cur_index] + 1;

            } else if (kMe == prev_owner) {
                ships_to_take_over_at_[cur_index] = enemy_arrivals_at_[cur_index] 
                                                    - my_arrivals_at_[cur_index]
                                                    - base_ships;
            } else { //kEnemy == prev_owner
                ships_to_take_over_at_[cur_index] = enemy_arrivals_at_[cur_index] 
                                                    + base_ships
                                                    - my_arrivals_at_[cur_index] + 1;
            }
        }

        ships_gained_at_[cur_index] = PlanetShipsGainRate(cur_owner, growth_rate);
    } //End iterating over turns.

    //Finish calculations of number of ships at each move necessary to make
    //produce an increase in the number of ships gained over the horizon.
    //At any point when the planet is under my control, check whether it could
    //use some reinforcing ships to fight a battle coming up a few turns later.
    //While we're at it, check whether the planet will not be mine at any point.
    int prev_ships_to_take_over = 0;
    will_not_be_mine_;

    for (int i = horizon_ - 1; i >= 0; --i) {
        const int cur_index = this->ActualIndex(i);

        if (i == horizon_ - 1) {
            prev_ships_to_take_over = ships_to_take_over_at_[cur_index];
            //No changes need to be made the the ships gained.

        } else {
            if (0 == ships_to_take_over_at_[cur_index]) {
                ships_to_take_over_at_[cur_index] = prev_ships_to_take_over;
            
            } else {
                prev_ships_to_take_over = ships_to_take_over_at_[cur_index];
                will_not_be_mine_ = true;
            }
        }
    }

    //Tally up the total number of ships gained over the forecast horizon.
    ships_gained_ = 0;

    for (unsigned int i = 0; i < ships_gained_at_.size(); ++i) {
        ships_gained_ += ships_gained_at_[i];
    }
}