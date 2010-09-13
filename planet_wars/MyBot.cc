#include <iostream>
#include "PlanetWars.h"

// The DoTurn function is where your code goes. The PlanetWars object contains
// the state of the game, including information about all planets and fleets
// that currently exist. Inside this function, you issue orders using the
// pw.IssueOrder() function. For example, to send 10 ships from planet 3 to
// planet 8, you would say pw.IssueOrder(3, 8, 10).
//
// There is already a basic strategy in place here. You can use it as a
// starting point, or you can throw it out entirely and replace it with your
// own. Check out the tutorials and articles on the contest website at
// http://www.ai-contest.com/resources.
void DoTurn(const PlanetWars& game_map) {
/*    //A simple starting strategy: sort the non-owned planets by 
    //"planet score", and send fleets to those that can be
    //captured.
    
    //Get the list of all planets
    std::vector<Planet> not_my_planets = game_map.NotMyPlanets();
    std::vector<Planet> my_planets = game_map.MyPlanets();
    const int num_not_my_planets = static_cast<int>(not_my_planets.size());
    const int num_my_planets = static_cast<int>(my_planets.size());
    
    //Compile the list of available ships.
    std::vector<int> remaining_ships_on_planets;

    for (int i = 0; i < num_my_planets; ++i) {
        remaining_ships_on_planets.push_back(my_planets[i].NumShips());
    }
    
    //Allocate ships to some of the planets by picking
    //the planet with the highest projected return in ships over the target
    //time horizon.
    const int time_horizon = 50;
    std::vector<double> planet_scores;
    planet_scores.reserve(num_not_my_planets.size());
    
    for (int p = 0; p < num_not_my_planets; ++p) {
        //Figure out how many steps it will take to take this planet.
        
        
        for 

*/

  // (2) Find my strongest planet.
  int source = -1;
  double source_score = -999999.0;
  int source_num_ships = 0;
  std::vector<Planet> my_planets = game_map.MyPlanets();
  for (int i = 0; i < my_planets.size(); ++i) {
    const Planet& p = my_planets[i];
    double score = (double)p.NumShips();
    if (score > source_score) {
      source_score = score;
      source = p.PlanetID();
      source_num_ships = p.NumShips();
    }
  }
  // (3) Find the weakest enemy or neutral planet.
  int dest = -1;
  double dest_score = -999999.0;
  std::vector<Planet> not_my_planets = game_map.NotMyPlanets();
  for (int i = 0; i < not_my_planets.size(); ++i) {
    const Planet& p = not_my_planets[i];
    double score = 1.0 / (1 + p.NumShips());
    if (score > dest_score) {
      dest_score = score;
      dest = p.PlanetID();
    }
  }
  // (4) Send half the ships from my strongest planet to the weakest
  // planet that I do not own.
  if (source >= 0 && dest >= 0) {
    int num_ships = source_num_ships / 2;
    game_map.IssueOrder(source, dest, num_ships);
  }
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
    PlanetMap planet_map;
    
    std::string current_line;
    std::string map_data;
    
    int turn = 0;

    while (true) {
        turn++;
        
        //Read the updated game state.
        int c = std::cin.get();
        current_line += (char)c;
        
        if (c == '\n') {
            if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
                //Initialize the new game state.
                PlanetWars planet_wars(map_data);
                map_data = "";
                
                //On the first turn, initialize the planetary map.
                if (1 == turn) {
                    planet_map.initialize(planet_wars.Planets());
                }
                
                //Make the moves.
                DoTurn(planet_wars);
                planet_wars.FinishTurn();
            
            } else {
                map_data += current_line;
            }

            current_line = "";
        }
    }

    return 0;
}
