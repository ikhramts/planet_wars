#include <iostream>
#include <list>
#include "Bot.h"
#include "PlanetWars.h"

Bot* gBot = NULL;

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
void DoTurn(const GameMap& game_map) {
    if (game_map.Turn() == 1) {
        gBot = new Bot();
        gBot->SetGame(&game_map);
    }

    FleetList fleets_to_send = gBot->MakeMoves();
    
    for (unsigned int i = 0; i < fleets_to_send.size(); ++i) {
        std::cout << fleets_to_send[i]->ToMoveOrder() << std::endl;
    }

    std::cout << "go" << std::endl;
    std::cout.flush();
}

// This is just the main game loop that takes care of communicating with the
// game engine for you. You don't have to understand or change the code below.
int main(int argc, char *argv[]) {
    GameMap game_map;
    
    std::string current_line;
    std::string map_data;
    
    int turn = 0;

    while (true) {
        //Read the updated game state.
        int c = std::cin.get();
        current_line += (char)c;
        
        if (c == '\n') {
            if (current_line.length() >= 2 && current_line.substr(0, 2) == "go") {
                turn++;
                
                //On the first turn, initialize the game map.
                //On later turns, just update it.
                if (1 == turn) {
                    game_map.Initialize(map_data);
                
                } else {
                    game_map.Update(map_data);
                }
                
                map_data = "";
                
                //Make the moves.
                DoTurn(game_map);
            
            } else {
                map_data += current_line;
            }

            current_line = "";
        }
    }

    return 0;
}
