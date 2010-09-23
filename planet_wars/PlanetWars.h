// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>
#include "Utils.h"

//Pre-define classes.
class StringUtil;
class Fleet;
class Planet;
class PlanetMap;
class GameMap;
class MoveOrder;

static const int kNeutral = 0;
static const int kMe = 1;
static const int kEnemy = 2;

typedef std::vector<Planet*> PlanetList;
typedef std::vector<Fleet*> FleetList;

// This is a utility class that parses strings.
class StringUtil {
public:
    // Tokenizes a string s into tokens. Tokens are delimited by any of the
    // characters in delimiters. Blank tokens are omitted.
    static void Tokenize(const std::string& s,
                         const std::string& delimiters,
                         std::vector<std::string>& tokens);

    // A more convenient way of calling the Tokenize() method.
    static std::vector<std::string> Tokenize(
                         const std::string& s,
                         const std::string& delimiters = std::string(" "));
};

// This class stores details about one fleet. There is one of these classes
// for each fleet that is in flight at any given time.
class Fleet {
public:
    // Initializes a fleet.
    Fleet();

    Fleet(int owner,
          int num_ships = 0,
          Planet* source_planet = NULL,
          Planet* destination_planet = NULL,
          int total_trip_length = -1,
          int turns_remaining = -1);
    
    void SetOwner(int owner)                    {owner_ = owner;}
    void SetNumShips(int num_ships)             {num_ships_ = num_ships;}
    void SetSource(Planet* planet)              {source_ = planet;}
    void SetDestination(Planet* planet)         {destination_ = planet;}
    void SetTripLength(int trip_length)         {trip_length_ = trip_length; turns_remaining_ = trip_length;}
    void SetTurnsRemaining(int turns_remaining) {turns_remaining_ = turns_remaining;}

    int Owner() const                           {return owner_;}
    int NumShips() const                        {return num_ships_;}
    Planet* Source() const                      {return source_;}
    Planet* Destination() const                 {return destination_;}
    int TripLength() const                      {return trip_length_;}
    int TurnsRemaining() const                  {return turns_remaining_;}

    //Convert this fleet to a move order that can be accepted by the game engine.
    std::string ToMoveOrder() const;

private:
    int owner_;
    int num_ships_;
    Planet* source_;
    Planet* destination_;
    int trip_length_;
    int turns_remaining_;
};

// Stores information about one planet. There is one instance of this class
// for each planet on the map.
class Planet {
public:
    // Initializes a planet.
    Planet(int planet_id,
           int owner,
           int num_ships,
           int growth_rate,
           double x,
           double y);
    
    // Returns the ID of this planets. Planets are numbered starting at zero.
    int Id() const;

    // Returns the ID of the player that owns this planet. Your playerID is
    // always 1. If the owner is 1, this is your planet. If the owner is 0, then
    // the planet is neutral. If the owner is 2 or some other number, then this
    // planet belongs to the enemy.
    int Owner() const;
    bool IsEnemys() const;
    bool IsNeutral() const;
    bool IsMine() const;

    // The number of ships on the planet. This is the "population" of the planet.
    int NumShips() const;

    // Returns the growth rate of the planet. Unless the planet is neutral, the
    // population of the planet grows by this amount each turn. The higher this
    // number is, the faster this planet produces ships.
    int GrowthRate() const;

    // The position of the planet in space.
    double X() const;
    double Y() const;

    // Use the following functions to set the properties of this planet. Note
    // that these functions only affect your program's copy of the game state.
    // You can't steal your opponent's planets just by changing the owner to 1
    // using the Owner(int) function! :-)
    void Owner(int new_owner);
    void NumShips(int new_num_ships);
    void AddShips(int amount);
    void RemoveShips(int amount);

private:
    int planet_id_;
    int owner_;
    int num_ships_;
    int growth_rate_;
    double x_, y_;
};

class GameMap {
public:
    // Initializes the game state given a string containing game state data.
    GameMap();
    
    //Initialize/Update the game state after start of a new turn.
    int Initialize(const std::string& game_state);
    int Update(const std::string& game_state);

    // Returns the number of planets on the map. Planets are numbered starting
    // with 0.
    int NumPlanets() const;

    // Returns the planet with the given planet_id. There are NumPlanets()
    // planets. They are numbered starting at 0.
    Planet* GetPlanet(int planet_id) const;

    // Returns the number of fleets.
    int NumFleets() const;

    // Returns the fleet with the given fleet_id. Fleets are numbered starting
    // with 0. There are NumFleets() fleets. fleet_id's are not consistent from
    // one turn to the next.
    Fleet* GetFleet(int fleet_id) const;

    // Returns a list of all the planets.
    // Get various lists of planets.
    PlanetList Planets() const;
    PlanetList MyPlanets() const;
    PlanetList NeutralPlanets() const;
    PlanetList EnemyPlanets() const;
    PlanetList NotMyPlanets() const;
    
    //Get planets sorted by distance from a certain planet.
    PlanetList PlanetsByDistance(Planet* origin);
    PlanetList MyPlanetsByDistance(Planet* origin);
    PlanetList NotMyPlanetsByDistance(Planet* origin);

    // Get various lists of fleets.
    FleetList Fleets() const;
    FleetList MyFleets() const;
    FleetList EnemyFleets() const;

    // Writes a string which represents the current game state. This string
    // conforms to the Point-in-Time format from the project Wiki.
    std::string ToString() const;

    // Returns the distance between two planets, rounded up to the next highest
    // integer. This is the number of discrete time steps it takes to get between
    // the two planets.
    int GetDistance(int source_planet, int destination_planet) const;
    int GetDistance(Planet* first_planet, Planet* second_planet) const;

    //Find the distance between two farthest planets.
    int MapRadius() const;

    // Sends an order to the game engine. The order is to send num_ships ships
    // from source_planet to destination_planet. The order must be valid, or
    // else your bot will get kicked and lose the game. For example, you must own
    // source_planet, and you can't send more ships than you actually have on
    // that planet.
    void IssueOrder(int source_planet,
	  	  int destination_planet,
	  	  int num_ships) const;

    // Returns true if the named player owns at least one planet or fleet.
    // Otherwise, the player is deemed to be dead and false is returned.
    bool IsAlive(int player_id) const;

    // Returns the number of ships that the given player has, either located
    // on planets or in flight.
    int NumShips(int player_id) const;

    // Sends a message to the game engine letting it know that you're done
    // issuing orders for now.
    void FinishTurn() const;

    //Find the list of fleets heading towards the planet.
    //Sort them by time to arrival to the destination.
    FleetList FleetsArrivingAt(Planet* destination) const;

    //Get current turn.
    int Turn() const            {return turn_;}

private:
    // Store all the planets and fleets. OMG we wouldn't wanna lose all the
    // planets and fleets, would we!?
    PlanetList planets_;
    FleetList fleets_;
    
    //Distances between individual planets.
    //The distance between source and destination is stored
    //in element (source_id * num_planets + destination_id).
    //There are num_planets^2 entries.
    std::vector<int> planet_distances_;
    int num_planets_;
    
    //Lists of planets sorted by distance from each other.
    //A list of planets sorted by distance from source_id starts at
    //element (source_id * num_planets) and ends at ((source_id+1) * num_planets - 1).
    PlanetList planets_by_distance_;

    //Fleets sorted by their destination.
    std::vector<FleetList> fleets_by_destination_;

    //Current turn.
    int turn_;
};

/************************************************
               General calculations
************************************************/

//Calculating the ships gained per turn.
int PlanetShipsGainRate(int owner, int growth_rate);

//Fight resolution.
struct BattleOutcome {
    int owner;
    int ships_remaining;
};

BattleOutcome ResolveBattle(int starting_owner, int neutral_ships, int my_ships, int enemy_ships);

#endif
