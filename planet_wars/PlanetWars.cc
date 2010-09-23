#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "PlanetWars.h"
#include "Utils.h"

/************************************************
                 StringUtil class
************************************************/
void StringUtil::Tokenize(const std::string& s,
                          const std::string& delimiters,
                          std::vector<std::string>& tokens) {
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

std::vector<std::string> StringUtil::Tokenize(const std::string& s,
                                              const std::string& delimiters) {
    std::vector<std::string> tokens;
    Tokenize(s, delimiters, tokens);
    return tokens;
}

/************************************************
                 Fleet class
************************************************/
Fleet::Fleet() 
: owner_(kMe),
num_ships_(0),
source_(NULL),
destination_(NULL),
trip_length_(-1),
turns_remaining_(-1) {
}


Fleet::Fleet(int owner,
             int num_ships,
             Planet* source_planet,
             Planet* destination_planet,
             int total_trip_length,
             int turns_remaining) {
    owner_ = owner;
    num_ships_ = num_ships;
    source_ = source_planet;
    destination_ = destination_planet;
    trip_length_ = total_trip_length;
    turns_remaining_ = turns_remaining;
}

std::string Fleet::ToMoveOrder() const {
    assert(NULL != source_);
    assert(NULL != destination_);

    std::stringstream moveOrder;
    moveOrder << source_->Id() << " " << destination_->Id() << " " << num_ships_;
    return moveOrder.str();
}

/************************************************
                 Planet class
************************************************/
Planet::Planet(int planet_id,
               int owner,
               int num_ships,
               int growth_rate,
               double x,
               double y) {
    planet_id_ = planet_id;
    owner_ = owner;
    num_ships_ = num_ships;
    growth_rate_ = growth_rate;
    x_ = x;
    y_ = y;
}

int Planet::Id() const {
    return planet_id_;
}

int Planet::Owner() const {
    return owner_;
}

bool Planet::IsEnemys() const {
    return (kEnemy == owner_);
}

bool Planet::IsNeutral() const {
    return (kNeutral == owner_);
}

bool Planet::IsMine() const {
    return (kMe == owner_);
}

int Planet::NumShips() const {
    return num_ships_;
}

int Planet::GrowthRate() const {
    return growth_rate_;
}

double Planet::X() const {
    return x_;
}

double Planet::Y() const {
    return y_;
}

void Planet::Owner(int new_owner) {
    owner_ = new_owner;
}

void Planet::NumShips(int new_num_ships) {
    num_ships_ = new_num_ships;
}

void Planet::AddShips(int amount) {
    num_ships_ += amount;
}

void Planet::RemoveShips(int amount) {
    num_ships_ -= amount;
}

/************************************************
               GameMap class
************************************************/
GameMap::GameMap() {
}

int GameMap::NumPlanets() const {
    return planets_.size();
}

Planet* GameMap::GetPlanet(int planet_id) const {
    return planets_[planet_id];
}

int GameMap::NumFleets() const {
    return fleets_.size();
}

Fleet* GameMap::GetFleet(int fleet_id) const {
    return fleets_[fleet_id];
}

PlanetList GameMap::Planets() const {
    return planets_;
}

PlanetList GameMap::MyPlanets() const {
    PlanetList r;
    for (int i = 0; i < num_planets_; ++i) {
        Planet* p = planets_[i];
        if (p->Owner() == kMe) {
            r.push_back(p);
        }
    }
    return r;
}

PlanetList GameMap::NeutralPlanets() const {
    PlanetList r;
    for (int i = 0; i < num_planets_; ++i) {
        Planet* p = planets_[i];
        if (p->Owner() == kNeutral) {
            r.push_back(p);
        }
    }
    return r;
}

PlanetList GameMap::EnemyPlanets() const {
    PlanetList r;
    for (int i = 0; i < num_planets_; ++i) {
        Planet* p = planets_[i];
        if (p->Owner() == kEnemy) {
            r.push_back(p);
        }
    }
    return r;
}

PlanetList GameMap::NotMyPlanets() const {
    PlanetList r;
    for (int i = 0; i < num_planets_; ++i) {
        Planet* p = planets_[i];
        if (p->Owner() != kMe) {
            r.push_back(p);
        }
    }
    return r;
}

PlanetList GameMap::PlanetsByDistance(Planet* origin) {
    const int origin_id = origin->Id();
    const int offset = origin_id * num_planets_;
    
    PlanetList planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;
        planets_by_distance.push_back(planets_by_distance_[planet_index]);
    }

    return planets_by_distance;
}

PlanetList GameMap::MyPlanetsByDistance(Planet* origin) {
    const int origin_id = origin->Id();
    const int offset = origin_id * num_planets_;
    
    PlanetList planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;

        if (planets_by_distance_[planet_index]->IsMine()) {
            planets_by_distance.push_back(planets_by_distance_[planet_index]);
        }
    }

    return planets_by_distance;
}

PlanetList GameMap::NotMyPlanetsByDistance(Planet* origin) {
    const int origin_id = origin->Id();
    const int offset = origin_id * num_planets_;
    
    PlanetList planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;

        if (!planets_by_distance_[planet_index]->IsMine()) {
            planets_by_distance.push_back(planets_by_distance_[planet_index]);
        }
    }

    return planets_by_distance;
}

FleetList GameMap::Fleets() const {
    return fleets_;
}

FleetList GameMap::MyFleets() const {
    FleetList r;
    const int num_fleets = static_cast<int>(fleets_.size());
    
    for (int i = 0; i < num_fleets; ++i) {
        Fleet* f = fleets_[i];
        if (f->Owner() == kMe) {
            r.push_back(f);
        }
    }
    return r;
}

FleetList GameMap::EnemyFleets() const {
    FleetList r;
    const int num_fleets = static_cast<int>(fleets_.size());
    
    for (int i = 0; i < num_fleets; ++i) {
        Fleet* f = fleets_[i];
        if (f->Owner() == kEnemy) {
            r.push_back(f);
        }
    }
    return r;
}

std::string GameMap::ToString() const {
    std::stringstream s;

    for (unsigned int i = 0; i < planets_.size(); ++i) {
        Planet* p = planets_[i];
        s << "P " << p->X() << " " << p->Y() << " " << p->Owner()
          << " " << p->NumShips() << " " << p->GrowthRate() << std::endl;
    }

    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        Fleet* f = fleets_[i];
        s << "F " << f->Owner() << " " << f->NumShips() << " "
          << f->Source()->Id() << " " << f->Destination()->Id() << " "
          << f->TripLength() << " " << f->TurnsRemaining() << std::endl;
    }

    return s.str();
}

int GameMap::GetDistance(int source_planet_id, int destination_planet_id) const {
    const int pair_index = source_planet_id * num_planets_ + destination_planet_id;
    const int distance = planet_distances_[pair_index];
    return distance;
}

int GameMap::GetDistance(Planet* first_planet, Planet* second_planet) const {
    const int first_planet_id = first_planet->Id();
    const int second_planet_id = second_planet->Id();
    const int distance = this->GetDistance(first_planet_id, second_planet_id);
    return distance;
}

void GameMap::IssueOrder(int source_planet,
                            int destination_planet,
                            int num_ships) const {
    std::cout << source_planet << " "
              << destination_planet << " "
              << num_ships << std::endl;
    std::cout.flush();
}

bool GameMap::IsAlive(int player_id) const {
    for (unsigned int i = 0; i < planets_.size(); ++i) {
        if (planets_[i]->Owner() == player_id) {
            return true;
        }
    }
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        if (fleets_[i]->Owner() == player_id) {
            return true;
        }
    }
    return false;
}

int GameMap::NumShips(int player_id) const {
    int num_ships = 0;

    for (unsigned int i = 0; i < planets_.size(); ++i) {
        if (planets_[i]->Owner() == player_id) {
            num_ships += planets_[i]->NumShips();
        }
    }
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        if (fleets_[i]->Owner() == player_id) {
            num_ships += fleets_[i]->NumShips();
        }
    }
    return num_ships;
}

//A functor to be used in the following function.
struct DistanceComparer {
    GameMap* game_map_;
    int origin_id;
    
    //Return true if the first planet at least as close to the origin planet
    //than the second planet.
    bool operator() (Planet* first_planet , Planet* second_planet) {
        const int first_planet_id = first_planet->Id();
        const int second_planet_id = second_planet->Id();
        const int first_distance = game_map_->GetDistance(origin_id, first_planet_id);
        const int second_distance = game_map_->GetDistance(origin_id, second_planet_id);
        return (first_distance < second_distance);
    }
};

int GameMap::Initialize(const std::string& s) {
    turn_ = 1;

    //Parse the game data.
    planets_.clear();
    fleets_.clear();
    std::vector<std::string> lines = StringUtil::Tokenize(s, "\n");
    int planet_id = 0;

    std::vector<int> fleet_source_ids;
    std::vector<int> fleet_destination_ids;

    for (unsigned int i = 0; i < lines.size(); ++i) {
        std::string& line = lines[i];
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }

        std::vector<std::string> tokens = StringUtil::Tokenize(line);
        if (tokens.size() == 0) {
            continue;
        }

        if (tokens[0] == "P") {
            if (tokens.size() != 6) {
                return 0;
            }
            
            Planet* p = new Planet(planet_id++,              // The ID of this planet
	                 atoi(tokens[3].c_str()),  // Owner
                     atoi(tokens[4].c_str()),  // Num ships
                     atoi(tokens[5].c_str()),  // Growth rate
                     atof(tokens[1].c_str()),  // X
                     atof(tokens[2].c_str())); // Y
            planets_.push_back(p);
        
        } else if (tokens[0] == "F") {
            if (tokens.size() != 7) {
                return 0;
            }
            
            Fleet* fleet = new Fleet();
            fleet->SetOwner(atoi(tokens[1].c_str()));
            fleet->SetNumShips(atoi(tokens[2].c_str()));
            fleet_source_ids.push_back(atoi(tokens[3].c_str()));
            fleet_destination_ids.push_back(atoi(tokens[4].c_str()));
            fleet->SetTripLength(atoi(tokens[5].c_str()));
            fleet->SetTurnsRemaining(atoi(tokens[6].c_str()));

            fleets_.push_back(fleet);
        
        } else {
            return 0;
        }
    }

    //Pre-calculate the distances between the planets.
    num_planets_ = static_cast<int>(planets_.size());
    planet_distances_.reserve(num_planets_ * num_planets_);

    for (int origin = 0; origin < num_planets_; ++origin) {
        for (int destination = 0; destination < num_planets_; ++destination) {
            const double dx = planets_[origin]->X() - planets_[destination]->X();
            const double dy = planets_[origin]->Y() - planets_[destination]->Y();

            const int distance = static_cast<int>(ceil(sqrt(dx * dx + dy * dy)));

            planet_distances_.push_back(distance);
        }
    }
    
    //Resolve planet references within the fleets and assign fleets
    //to their destinations.
    fleets_by_destination_.resize(planets_.size());

    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        Fleet* fleet = fleets_[i];
        const int destination_id = fleet_destination_ids[i];

        fleet->SetSource(planets_[fleet_source_ids[i]]);
        fleet->SetDestination(planets_[destination_id]);
        
        fleets_by_destination_[destination_id].push_back(fleet);
    }

    //Pre-sort planets by distance from each other.
    //Use a functor defined immediately before this function.
    DistanceComparer distance_comparer;
    distance_comparer.game_map_ = this;
    distance_comparer.origin_id = 0;

    planets_by_distance_.clear();
    planets_by_distance_.reserve(num_planets_ * num_planets_);
    
    for (planet_id = 0; planet_id < num_planets_; ++planet_id) {
        //Sort the planets by distance from the origin planet and
        //append the result to the general vector of planets sorted
        //by distance.
        std::vector<Planet*> planets_to_sort(planets_);
        distance_comparer.origin_id = planet_id;
        std::sort(planets_to_sort.begin(), planets_to_sort.end(), distance_comparer);
        planets_by_distance_.insert(planets_by_distance_.end(),
            planets_to_sort.begin(), planets_to_sort.end());
    }
    
    return 1;
}

int GameMap::Update(const std::string& s) {
    ++turn_;

    //Update the planet data; repopulate the fleets.
    fleets_.clear();
    std::vector<std::string> lines = StringUtil::Tokenize(s, "\n");
    int planet_id = 0;

    std::vector<int> fleet_source_ids;
    std::vector<int> fleet_destination_ids;

    for (unsigned int i = 0; i < lines.size(); ++i) {
        std::string& line = lines[i];
        size_t comment_begin = line.find_first_of('#');
        if (comment_begin != std::string::npos) {
            line = line.substr(0, comment_begin);
        }

        std::vector<std::string> tokens = StringUtil::Tokenize(line);
        if (tokens.size() == 0) {
            continue;
        }
        
        if (tokens[0] == "P") {
            //Read a line describing a planet
            if (tokens.size() != 6) {
                return 0;
            }
            
            //Update the planet state.
            Planet* planet = planets_[planet_id];
            planet->Owner(atoi(tokens[3].c_str()));
            planet->NumShips(atoi(tokens[4].c_str()));

            planet_id++;
            
        } else if (tokens[0] == "F") {
            //Read a line describing a fleet
            if (tokens.size() != 7) {
                return 0;
            }

            Fleet* fleet = new Fleet();
            fleet->SetOwner(atoi(tokens[1].c_str()));
            fleet->SetNumShips(atoi(tokens[2].c_str()));
            fleet_source_ids.push_back(atoi(tokens[3].c_str()));
            fleet_destination_ids.push_back(atoi(tokens[4].c_str()));
            fleet->SetTripLength(atoi(tokens[5].c_str()));
            fleet->SetTurnsRemaining(atoi(tokens[6].c_str()));

            fleets_.push_back(fleet);

        } else {
            return 0;
        }
    }
    
    //Clear the fleets sorted by destination.
    for (uint i = 0; i < fleets_by_destination_.size(); ++i) {
        fleets_by_destination_[i].clear();
    }

    //Resolve planet references within the fleets, and sort the fleets
    //by their destination.
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        Fleet* fleet = fleets_[i];
        const int destination_id = fleet_destination_ids[i];

        fleet->SetSource(planets_[fleet_source_ids[i]]);
        fleet->SetDestination(planets_[destination_id]);
        
        fleets_by_destination_[destination_id].push_back(fleet);
    }

    return 1;
}

void GameMap::FinishTurn() const {
    std::cout << "go" << std::endl;
    std::cout.flush();
}

FleetList GameMap::FleetsArrivingAt(Planet *destination) const {
    return fleets_by_destination_[destination->Id()];
}
/************************************************
               General calculations
************************************************/
int PlanetShipsGainRate(int owner, int growth_rate) {
    const int gain_multiplier = (owner + 1) % 3 - 1;
    const int gain_rate = growth_rate * gain_multiplier;
    return gain_rate;
}

/************************************************
               Battle resolution.
************************************************/
BattleOutcome ResolveBattle(int starting_owner, int neutral_ships, int my_ships, int enemy_ships) {
    BattleOutcome outcome;
    
    if (neutral_ships != 0) {
        //Planet was neutral to begin with.
        const int max_me_or_enemy = std::max(my_ships, enemy_ships);

        if (neutral_ships > max_me_or_enemy) {
            outcome.ships_remaining = neutral_ships - max_me_or_enemy;
            outcome.owner = kNeutral;

        } else {
            if (my_ships > enemy_ships) {
                outcome.ships_remaining = my_ships - std::max(neutral_ships, enemy_ships);
                outcome.owner = kMe;

            } else if (my_ships < enemy_ships) {
                outcome.ships_remaining = enemy_ships - std::max(my_ships, enemy_ships);
                outcome.owner = kEnemy;
            
            } else {
                outcome.ships_remaining = 0;
                outcome.owner = kNeutral;
            }
        }

    } else {
        //Planet was owned by someone.
        if (my_ships > enemy_ships) {
            outcome.ships_remaining = my_ships - enemy_ships;
            outcome.owner = kMe;

        } else if (my_ships < enemy_ships) {
            outcome.ships_remaining = enemy_ships - my_ships;
            outcome.owner = kEnemy;

        } else {
            outcome.ships_remaining = 0;
            outcome.owner = starting_owner;
        }
    } //if (neutral_ships != 0)

    return outcome;
}