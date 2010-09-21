//Available under GPLv3.
//Author: Iouri Khramtsov.

#include "GameForecaster.h"

/************************************************
               GameForecaster class
************************************************/
GameForecaster::GameForecaster()
: game_(NULL) {
}

void GameForecaster::SetGameMap(GameMap* game) {
    game_ = game;

    //Update the ships gained from planets vector.
    ships_gained_from_planets_ = std::vector(game_->Planets().size(), 0);
}

void GameForecaster::Update() {
    //Reset the list of planets changing hands.
    planets_not_mine_at_least_once_.clear();
    
    //Update the planet data.
    PlanetList planets = game_->Planets();

    for (unsigned int i = 0; i < planets.size(); ++i) {
        this->UpdatePlanetForecast(planets[i], kTimeHorizon);
    }
}

int GameForecaster::ShipsGainedFrom(Planet* planet) const {
    const int planet_id = planet->Id();
    const int ships_gained = ships_gained_from_planets_[planet_id];
    return ships_gained;
}

void GameForecaster::UpdatePlanetForecast(Planet* planet, int num_turns_ahead) {
    //Find the fleets heading towards the planet. 
    //They should already be sorted by time remaining.
    FleetList arriving_fleets = this->FleetsArrivingAt(planet);
    
    bool not_mine_at_least_once = false;

    //Calculate the number of fleets produced by the planet between each
    //fleet arrivals.
    int ships_gained = 0;
    int ships_on_planet = planet->NumShips();

    int growth_period_start = 0;
    int growth_period_end = 0;

    int arriving_fleets_start = 0;
    int arriving_fleets_end = 0;

    const int num_arriving_fleets = static_cast<int>(arriving_fleets.size());
    int owner = planet->Owner();
    
    while(growth_period_start < num_turns_ahead) {
        //Calculate how long this period of planetary ownership might last.
        growth_period_end = num_turns_ahead;

        if (arriving_fleets_start < num_arriving_fleets) {
            const int fleet_arrives_at = arriving_fleets[arriving_fleets_start]->TurnsRemaining();
            growth_period_end = std::min(num_turns_ahead, fleet_arrives_at);
        }

        //Calculate the number of ships produced over this period.
        const int period_production = destination->NumShipsOver(growth_period_start, growth_period_end);
        const int period_gain = period_production * ((owner + 1) % 3 - 1);
        ships_gained += period_gain;
        ships_on_planet += (kNeutral == owner ? 0 : period_production);

        //Update whether the planet is not mine at any point.
        if (kMe != owner) {
            not_mine_at_least_once = true;
        }

        //Check whether the ownership of the planet will change at the end of this period.
        if (arriving_fleets_start < num_arriving_fleets) {
            int neutral_ships = (kNeutral == owner ? ships_on_planet : 0);
            int my_ships = (kMe == owner ? ships_on_planet : 0);
            int enemy_ships = (kEnemy == owner ? ships_on_planet : 0);

            //Find all ships and fleets arriving at the end of this growth period.
            arriving_fleets_end = arriving_fleets_start;
            while (arriving_fleets_end < num_arriving_fleets) {
                if (arriving_fleets[arriving_fleets_end]->TurnsRemaining() == growth_period_end) {
                    Fleet* arriving_fleet = arriving_fleets[arriving_fleets_end];

                    if (arriving_fleet->Owner() == kMe) {
                        my_ships += arriving_fleet->NumShips();

                    } else {
                        enemy_ships += arriving_fleet->NumShips();
                    }

                } else {
                    break;
                }

                ++arriving_fleets_end;
            }

            arriving_fleets_start = arriving_fleets_end;

            //Calculate who wins.
            if (neutral_ships != 0) {
                //Planet was neutral to begin with.
                const int max_me_or_enemy = std::max(my_ships, enemy_ships);

                if (neutral_ships > max_me_or_enemy) {
                    ships_on_planet = neutral_ships - max_me_or_enemy;
                    //Planet stays neutral.

                } else {
                    if (my_ships > enemy_ships) {
                        ships_on_planet = my_ships - std::max(neutral_ships, enemy_ships);
                        owner = kMe;

                    } else if (my_ships < enemy_ships) {
                        ships_on_planet = enemy_ships - std::max(my_ships, enemy_ships);
                        owner = kEnemy;
                    
                    } else {
                        ships_on_planet = 0;
                        //Planet stays neutral.
                    }
                }

            } else {
                //Planet was owned by someone.
                if (my_ships > enemy_ships) {
                    ships_on_planet = my_ships - enemy_ships;
                    owner = kMe;

                } else if (my_ships < enemy_ships) {
                    ships_on_planet = enemy_ships - my_ships;
                    owner = kEnemy;

                } else {
                    ships_on_planet = 0;
                    //Owner stays as it was.
                }
            } //if (neutral_ships != 0)
        } // End checking ownership change.

        growth_period_start = growth_period_end;
    } //End iterating over growth periods.

    //Update the planet data based on what we just calculated.
    if (not_mine_at_least_once) {
        planets_not_mine_at_least_once_.push_back(planet);
    }

    ships_gained_from_planets_[planet->Id()] = ships_gained;
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
    start_ = 0;
    end_ = u_horizon;
    const size_t u_horizon = static_cast<size_t>(horizon_);
    owner_at_.resize(u_horizon, 0);
    ships_at_.resize(u_horizon, 0);
    my_arrivals_at_.resize(u_horizon, 0);
    enemy_arrivals_at_.resize(u_horizon, 0);
    ships_to_take_over_at_.resize(u_horizon, 1);
    
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

    this->CalculatePlanetStateProjections(0);
}

void PlanetForecaster::Update() {
    //Advance the forecast frame through time.
    ++start_;
    end = (start_ + horizon_ - 1) % horizon_;
    
    my_arrivals_at_[end] = 0;
    enemy_arrivals_at_[end] = 0;
    
    int start_update_at = horizon_ - 1;

    //Update the fleet arrivals.
    FleetList arrivingFleets = game->FleetsArrivingAt(planet);
    
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

    //Check whether a fleet managed to make a length-1 trip onto this planet.
    //It would have already landed, so it would not be caught in the above for loop.
    if (owner_at_[start] != planet_->Owner() || ships_at_[start] != planet_->NumShips()) {
        owner_at_[start] = planet_->Owner();
        ships_at_[start] = planet_->NumShips();
        start_update_at = 1;
    }

    this->CalculatePlanetStateProjections(start_update_at);
}

void PlanetForecaster::CalculatePlanetStateProjections(int starting_at) {
    const int growth_rate = planet_->GrowthRate();
    
    for (unsigned int i = starting_at; i < horizon_; ++i) {
        const int prev_index = (start_ + i - 1 + horizon_) % horizon_;
        const int cur_index = (start_ + i) % horizon_;
        const int prev_owner = owner_at_[prev_index];
        const int prev_ships = ships_at_[prev_index];

        const int base_ships = prev_ships + (kNeutral == prev_owner ? 0 : growth_rate);

        //Check whether any ships arrive at the planet in this turn.
        if (0 == my_arrivals_[cur_index] && 0 == enemy_arrivals_at_[cur_index]) {
            //Ships don't arrive.
            owner_at_[cur_index] = prev_owner;
            ships_at_[cur_index] = base_ships;

        } else {
            //A fleet has arrived.  Resolve the battle.
            neutral_ships = (kNeutral == prev_owner ? base_ships : 0);
            my_ships = my_arrivals_[cur_index] + (kMe == prev_owner ? base_ships : 0);
            enemy_ships = enemy_arrivals_at_[cur_index] + (kEnemy == prev_owner ? base_ships : 0);
            
            if (neutral_ships != 0) {
                //Planet was neutral to begin with.
                const int max_me_or_enemy = std::max(my_ships, enemy_ships);

                if (neutral_ships > max_me_or_enemy) {
                    ships_at_[cur_index] = neutral_ships - max_me_or_enemy;
                    owner_at_[cur_index] = kNeutral;

                } else {
                    if (my_ships > enemy_ships) {
                        ships_at_[cur_index] = my_ships - std::max(neutral_ships, enemy_ships);
                        owner_at_[cur_index] = kMe;

                    } else if (my_ships < enemy_ships) {
                        ships_at_[cur_index] = enemy_ships - std::max(my_ships, enemy_ships);
                        owner_at_[cur_index] = kEnemy;
                    
                    } else {
                        ships_at_[cur_index] = 0;
                        owner_at_[cur_index] = kNeutral;
                    }
                }

            } else {
                //Planet was owned by someone.
                if (my_ships > enemy_ships) {
                    ships_at_[cur_index] = my_ships - enemy_ships;
                    owner_at_[cur_index] = kMe;

                } else if (my_ships < enemy_ships) {
                    ships_at_[cur_index] = enemy_ships - my_ships;
                    owner_at_[cur_index] = kEnemy;

                } else {
                    ships_at_[cur_index] = 0;
                    owner_at_[cur_index] = prev_owner;
                }
            } //if (neutral_ships != 0)
        } // End battle resulution.
    
        //Check how many ships are necessary at this point to make the planet mine.
        const int cur_owner = owner_at_[cur_index];
        const int cur_ships = ships_at_[cur_index];

        if (kMe == cur_owner) {
            ships_to_take_over_at_[cur_index] = 0;

        } else if (kNeutral == cur_owner) {
            ships_to_take_over_at_[cur_index] = prev_ships - my_arrivals_[cur_index] + 1;

        } else {
            if (kNeutral == prev_owner) {
                ships_to_take_over_at_[cur_index] = 
                    enemy_arrivals_at_[cur_index] - my_arrivals_[cur_index] + 1;

            } else if (kMe == prev_owner) {
                ships_to_take_over_at_[cur_index] = enemy_arrivals_at_[cur_index] 
                                                    - my_arrivals_[cur_index]
                                                    - base_ships;
            } else { //kEnemy == prev_owner
                ships_to_take_over_at_[cur_index] = enemy_arrivals_at_[cur_index] 
                                                    + base_ships
                                                    - my_arrivals_[cur_index] + 1;
            }
        }
    } //End iterating over turns.
}