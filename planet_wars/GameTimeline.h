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
    //int ShipsGainedForFleets(const FleetList& fleets, Planet* planet) const;
    int ShipsGainedForActions(const ActionList& actions, Planet* planet) const;

    //Get the list of planets that will not be mine at any point in time
    //over the projected horizon.
    PlanetList PlanetsThatWillNotBeMine() const;
    PlanetTimelineList TimelinesThatWillNotBeMine() const;

    //Find the number of ships needed to make sure that the specified planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(Planet* planet, int arrival_time) const;

    //Return the minimum number of ships required to stay on the planet to win any
    //upcoming battles.
    int ShipsRequiredToKeep(Planet* planet, int arrival_time) const;

    PlanetTimelineList Timelines() const            {return planet_timelines_;}
    PlanetTimelineList TimelinesOwnedBy(int owner, int when = 0) const;
    PlanetTimelineList TimelinesNotOwnedBy(int owner, int when = 0) const;

	PlanetTimelineList EverOwnedTimelinesByDistance(int owner, PlanetTimeline* source);
    PlanetTimelineList OwnedTimelinesByDistance(int owner, PlanetTimeline* source, int when = 0);
	
	//Apply actions to the timeline, changing the forecasts.
	void ApplyActions(const ActionList& actions);

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
    //int ShipsGainedForFleets(const FleetList& fleets) const;
    int ShipsGainedForActions(const ActionList& actions) const;

    
    Planet* GetPlanet() const               {return planet_;}
    int Id() const                          {return id_;}

    //Find the number of ships needed to make sure that this planet
    //becomes/stays mine given that the ships will arrive in specified number
    //of turns.
    int ShipsRequredToPosess(int arrival_time) const;

    //Return the minimum number of ships required to stay on the planet to win any
    //upcoming battles.
    int ShipsRequiredToKeep(int arrival_time) const;

	//Find the number of ships available to be sent from this planet.
	int ShipsFree(int when) const;

    //Check whether the planet is forecast to be owned by a player at a specified date.
    bool IsOwnedBy(int owner, int when = 0) const;
    bool WillNotBeMine() const              {return will_not_be_mine_;}
	bool WillBeMine() const					{return will_be_mine_;}
	bool WillBeEnemys() const				{return will_be_enemys_;}
	bool WillBeOwnedBy(int owner) const;

    //Apply action to the timeline.
    void AddDeparture(Action* action);
    void AddArrivals(const ActionList& actions);

private:
    //Update the planet state projections.
    void CalculatePlanetStateProjections(int starting_at);

    //Calculate actual index of an element in the *_at_ vectors
    //given a plain index.
    int ActualIndex(int i) const        {return (i + start_ + horizon_) % horizon_; }

    int id_;        //Should be same as planet_id.
    int horizon_;
    int start_;
    int end_;
    std::vector<int> owner_;
    std::vector<int> ships_;
    std::vector<int> my_arrivals_;
    std::vector<int> enemy_arrivals_;
    std::vector<int> ships_to_take_over_;
    std::vector<int> ships_gained_;
    std::vector<int> ships_to_keep_;        //Minimum ships required to keep the planet.
    std::vector<int> ships_reserved_;
    std::vector<int> ships_free_;

    int total_ships_gained_;

    //Indicates whether the planet will not be mine at any point
    //in the evaluated time frame.
    bool will_not_be_mine_;
	bool will_be_enemys_;
	bool will_be_mine_;

    GameMap* game_;
    Planet* planet_;

    //Temporary storage.
    mutable std::vector<int> my_additional_arrivals_at_;
    mutable std::vector<int> enemy_additional_arrivals_at_;

};

#endif