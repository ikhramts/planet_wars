#include "PlanetWars.h"
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
             Planet* source_planet = NULL,
             Planet* destination_planet = NULL,,
             int total_trip_length,
             int turns_remaining) {
    owner_ = owner;
    num_ships_ = num_ships;
    source_ = source_planet;
    destination_ = destination_planet;
    trip_length_ = total_trip_length;
    turns_remaining_ = turns_remaining;
}

std::string Fleet::toMoveOrder() const {
    assert(NULL != source_);
    assert(NULL != destination_);

    std::stringstream moveOrder;
    moveOrder << source_->Id() << " " << destination_->Id() << " " num_ships_;
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
    return (2 == owner_);
}

bool Planet::IsNeutral() const {
    return (0 == owner_);
}

bool Planet::IsMine() const {
    return (1 == owner_);
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

void Planet::NumShipsIn(const int turns_from_now) {
    if (kNeutral == owner_) {
        return num_ships_;

    } else {
        const int num_ships = num_ships_ + turns_from_now * growth_rate_;
        return num_ships;
    }
}

void Planet::NumShipsOver(int from_step, int to_step) const {
    const int num_steps = to_step - from_step;
    const int produced = num_steps * growth_rate_;
    return produced;
}

/************************************************
               GameMap class
************************************************/
GameMap::GameMap() {
}

int GameMap::NumPlanets() const {
    return planets_.size();
}

const Planet& GameMap::GetPlanet(int planet_id) const {
    return planets_[planet_id];
}

int GameMap::NumFleets() const {
    return fleets_.size();
}

const Fleet& GameMap::GetFleet(int fleet_id) const {
    return fleets_[fleet_id];
}

std::vector<Planet> GameMap::Planets() const {
    std::vector<Planet> r;
    for (int i = 0; i < num_planets_; ++i) {
        const Planet& p = planets_[i];
        r.push_back(p);
    }
    return r;
}

std::vector<Planet*> GameMap::PlanetPointers() {
    std::vector<Planet*> result;
    result.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        result.push_back(&planets_[i]);
    }

    return result;
}

std::vector<Planet> GameMap::MyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < num_planets_; ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() == 1) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> GameMap::NeutralPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < num_planets_; ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() == 0) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> GameMap::EnemyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < num_planets_; ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() > 1) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet> GameMap::NotMyPlanets() const {
    std::vector<Planet> r;
    for (int i = 0; i < num_planets_; ++i) {
        const Planet& p = planets_[i];
        if (p.Owner() != 1) {
            r.push_back(p);
        }
    }
    return r;
}

std::vector<Planet*> GameMap::PlanetsByDistance(const Planet& origin) {
    const int origin_id = origin.Id();
    const int offset = origin_id * num_planets_;
    
    std::vector<Planet*> planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;
        planets_by_distance.push_back(planets_by_distance_[planet_index]);
    }

    return planets_by_distance;
}

std::vector<Planet*> GameMap::MyPlanetsByDistance(const Planet& origin) {
    const int origin_id = origin.Id();
    const int offset = origin_id * num_planets_;
    
    std::vector<Planet*> planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;

        if (planets_by_distance_[planet_index]->IsMine()) {
            planets_by_distance.push_back(planets_by_distance_[planet_index]);
        }
    }

    return planets_by_distance;
}

std::vector<Planet*> GameMap::NotMyPlanetsByDistance(const Planet& origin) {
    const int origin_id = origin.Id();
    const int offset = origin_id * num_planets_;
    
    std::vector<Planet*> planets_by_distance;
    planets_by_distance.reserve(planets_.size());

    for (int i = 0; i < num_planets_; ++i) {
        const int planet_index = offset + i;

        if (!planets_by_distance_[planet_index]->IsMine()) {
            planets_by_distance.push_back(planets_by_distance_[planet_index]);
        }
    }

    return planets_by_distance;
}

std::vector<Fleet> GameMap::Fleets() const {
    std::vector<Fleet> r;
    const int num_fleets = static_cast<int>(fleets_.size());
    
    for (int i = 0; i < num_fleets; ++i) {
        const Fleet& f = fleets_[i];
        r.push_back(f);
    }
    return r;
}

std::vector<Fleet> GameMap::MyFleets() const {
    std::vector<Fleet> r;
    const int num_fleets = static_cast<int>(fleets_.size());
    
    for (int i = 0; i < num_fleets; ++i) {
        const Fleet& f = fleets_[i];
        if (f.Owner() == 1) {
            r.push_back(f);
        }
    }
    return r;
}

std::vector<Fleet> GameMap::EnemyFleets() const {
    std::vector<Fleet> r;
    const int num_fleets = static_cast<int>(fleets_.size());
    
    for (int i = 0; i < num_fleets; ++i) {
        const Fleet& f = fleets_[i];
        if (f.Owner() > 1) {
            r.push_back(f);
        }
    }
    return r;
}

std::string GameMap::ToString() const {
    std::stringstream s;

    for (unsigned int i = 0; i < planets_.size(); ++i) {
        const Planet& p = planets_[i];
        s << "P " << p.X() << " " << p.Y() << " " << p.Owner()
          << " " << p.NumShips() << " " << p.GrowthRate() << std::endl;
    }

    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        const Fleet& f = fleets_[i];
        s << "F " << f.Owner() << " " << f.NumShips() << " "
          << f.SourcePlanet() << " " << f.DestinationPlanet() << " "
          << f.TotalTripLength() << " " << f.TurnsRemaining() << std::endl;
    }

    return s.str();
}

int GameMap::GetDistance(int source_planet_id, int destination_planet_id) const {
    const int pair_index = source_planet_id * num_planets_ + destination_planet_id;
    const int distance = planet_distances_[pair_index];
    return distance;
}

int GameMap::GetDistance(const Planet& first_planet, const Planet& second_planet) const {
    const int first_planet_id = first_planet.Id();
    const int second_planet_id = second_planet.Id();
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
        if (planets_[i].Owner() == player_id) {
            return true;
        }
    }
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        if (fleets_[i].Owner() == player_id) {
            return true;
        }
    }
    return false;
}

int GameMap::NumShips(int player_id) const {
    int num_ships = 0;
    for (unsigned int i = 0; i < planets_.size(); ++i) {
        if (planets_[i].Owner() == player_id) {
            num_ships += planets_[i].NumShips();
        }
    }
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        if (fleets_[i].Owner() == player_id) {
            num_ships += fleets_[i].NumShips();
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
    
    //Resolve planet references within the fleets.
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        Fleet* fleet = fleets_[i];
        
        fleet->SetSource(planets_[fleet_source_ids[i]]->Id());
        fleet->SetDestination(planets_[fleet_destination_ids[i]]->Id());
    }

    //Pre-calculate the distances between the planets.
    num_planets_ = static_cast<int>(planets_.size());

    //Initialize the distances between the planets.
    planet_distances_.reserve(num_planets_ * num_planets_);

    for (int origin = 0; origin < num_planets_; ++origin) {
        for (int destination = 0; destination < num_planets_; ++destination) {
            const double dx = planets_[origin].X() - planets_[destination].X();
            const double dy = planets_[origin].Y() - planets_[destination].Y();

            const int distance = static_cast<int>(ceil(sqrt(dx * dx + dy * dy)));

            planet_distances_.push_back(distance);
        }
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
        std::vector<Planet*> planets_to_sort(this->PlanetPointers());
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

    //Resolve planet references within the fleets.
    for (unsigned int i = 0; i < fleets_.size(); ++i) {
        Fleet* fleet = fleets_[i];
        
        fleet->SetSource(planets_[fleet_source_ids[i]]->Id());
        fleet->SetDestination(planets_[fleet_destination_ids[i]]->Id());
    }

    return 1;
}

void GameMap::FinishTurn() const {
    std::cout << "go" << std::endl;
    std::cout.flush();
}

void GameMap::FleetsArrivingAt(Planet *destination) const {
    //TODO: implement.
    //Remember: sort them by arrival time.
}
