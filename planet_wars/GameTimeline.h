//Available under GPLv3.
//Author: Iouri Khramtsov.

#ifndef PLANET_WARRS_GAME_FORECASTER_H_
#define PLANET_WARRS_GAME_FORECASTER_H_

#include <vector>
#include "Actions.h"
#include "PlanetWars.h"
#include "Utils.h"

class PlanetTimeline;

typedef std::vector<PlanetTimeline*> PlanetTimelineList;

//This class is responsible for forecasting the state of the game.
class GameTimeline {
public:
    GameTimeline();
    ~GameTimeline();

    void SetGameMap(GameMap* game);

	int Horizon() const						{return horizon_;}

    //Recalculate the forecasts given the state of the game map.
    void Update();

    //Calculate how many additional ships will be gained by sending a set
    //of fleets to a planet.
    int ShipsGainedForActions(const ActionList& actions, Planet* planet) const;

    //Get the list of planets that will not be mine at any point in time
    //over the projected horizon.
    PlanetList PlanetsThatWillNotBeMine() const;
    PlanetTimelineList TimelinesEverNotOwnedBy(int player) const;

    //Find the number of ships needed to make sure that the specified planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(Planet* planet, int arrival_time, int by_whom) const;

    PlanetTimelineList Timelines() const            {return planet_timelines_;}
    PlanetTimelineList TimelinesOwnedBy(int owner, int when = 0) const;
    PlanetTimelineList TimelinesNotOwnedBy(int owner, int when = 0) const;

	PlanetTimelineList EverOwnedTimelinesByDistance(int owner, PlanetTimeline* source);
    PlanetTimelineList OwnedTimelinesByDistance(int owner, PlanetTimeline* source, int when = 0);
	
	//Apply actions to the timeline, changing the forecasts.
	void ApplyActions(const ActionList& actions);

	//Remove actions from the timeline, changing the forecasts.
	void UnapplyActions(const ActionList& actions);

private:
    int horizon_;
    GameMap* game_;
    PlanetTimelineList planet_timelines_;
};

//A class for forecasting the state of each planet.
class PlanetTimeline {
public:
    PlanetTimeline();
    
    void Initialize(int forecast_horizon, Planet* planet, GameMap* game);
    
    void Update();

    //Calculate how many additional ships would be gained if specified
    //fleets would be sent to the planet.
    int ShipsGainedForActions(const ActionList& actions) const;

    
    Planet* GetPlanet() const               {return planet_;}
    int Id() const                          {return id_;}

    //Find the number of ships needed to make sure that this planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(int arrival_time, int by_whom) const;

	//Find the number of ships available to be sent from this planet.
	int ShipsFree(int when, int owner) const;

    //Check whether the planet is forecast to be owned by a player at a specified date.
    bool IsOwnedBy(int owner, int when = 0) const;
    bool WillNotBeMine() const              {return will_not_be_mine_;}
	bool WillBeMine() const	                {return will_be_mine_;}
	bool WillBeEnemys() const               {return will_be_enemys_;}
    bool WillNotBeEnemys() const            {return will_not_be_enemys_;}
	bool WillBeOwnedBy(int owner) const;
    bool WillNotBeOwnedBy(int player) const;

    //Apply actions to the timeline.
    void AddDeparture(Action* action);
    void AddArrivals(const ActionList& actions);
	
	//Remove actions from the timeline.  Unlike the Add*() above, these do 
	//not cause recalculations in planets' timelines.  RecalculateTimeline()
	//should be called to update the planet timelines.
	void RemoveDeparture(Action* action);
	void RemoveArrival(Action* action);
    
    //Reset various data before starting full timeline recalculation.
    void ResetStartingData();

    //Update the planet state projections.
    void RecalculateTimeline(int starting_at);

    //Set the planet as a reinforcer.  Reinforcers will never supply ships for an attack.
    void SetReinforcer(bool is_reinforcer)  {is_reinforcer_ = is_reinforcer;}
    bool IsReinforcer() const               {return is_reinforcer_;}

private:
    //Reserve ships for a departure or defense.
    void ReserveShips(int owner, int key_time, int num_ships);

    int id_;        //Should be same as planet_id.
    int horizon_;
    std::vector<int> owner_;
    std::vector<int> ships_;
    std::vector<int> my_arrivals_;
    std::vector<int> enemy_arrivals_;
    std::vector<int> ships_to_take_over_;
    std::vector<int> ships_gained_;
    std::vector<int> available_growth_;
    std::vector<int> ships_reserved_;
    std::vector<int> ships_free_;

    std::vector<int> enemy_ships_to_take_over_;
    std::vector<int> enemy_ships_reserved_;
    std::vector<int> enemy_ships_free_;
    std::vector<int> enemy_available_growth_;
    

    std::vector<int> my_departures_;
    std::vector<int> enemy_departures_;

    int total_ships_gained_;

    //Indicates whether the planet will not be mine at any point
    //in the evaluated time frame.
    bool will_not_be_enemys_;
    bool will_not_be_mine_;
	bool will_be_enemys_;
	bool will_be_mine_;

    GameMap* game_;
    Planet* planet_;

    //Temporary storage.
    mutable std::vector<int> additional_arrivals_;

    bool is_reinforcer_;
};

#endif
