//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file contains the main bot logic.

#ifndef PLANET_WARS_BOT_H_
#define PLANET_WARS_BOT_H_

#include "PlanetWars.h"
#include "GameTimeline.h"
#include "Actions.h"

class GameTimeline;
class CounterActionResult;

class Bot {
public:
    static const double kAggressionReturnMultiplier;

    Bot();
    ~Bot();

    //Initialization.
    void SetGame(GameMap* game);

    //Decide which fleets to send.  This is the main move logic function.
    ActionList MakeMoves();

private:
    ActionList FindActionsFor(int player);

    ActionList BestRemainingMove(PlanetTimelineList& invadeable_planets, 
                                 int player,
                                 int earliest_departure,
                                 const std::vector<int>& earliest_arrivals,
                                 const std::vector<int>& latest_arrivals,
                                 int depth = 0);
    
    ActionList FindInvasionPlan(PlanetTimeline* target, int arrival_time, 
        const PlanetTimelineList& sources_by_distance, const std::vector<int>& distances_to_sources,
        int player, int balance_adjustment = 0, int owner_adjustment -1);
    double ReturnForMove(const ActionList& invasion_plan, double best_return);
    double ReturnForMove2(ActionList& invasion_plan, double best_return, int depth);

    CounterActionResult ShipsGainedForAfterMove(const ActionList& invasion_plan, 
                                         PlanetTimelineList& counter_targets,
                                         const int attack_arrival_time);

    ActionList SendFleetsToFront(int player);

    //Mark feeder planets as such.
    void MarkReinforcers(int player);

    ActionList FindSupportPlanFor(PlanetTimeline *planet, int player);
    double ReturnOnSupportPlan(const ActionList& support_plan);
    
    GameMap* game_;
    GameTimeline* timeline_;
    int counter_horizon_;
    int defense_horizon_;
    int turn_;
    int picking_round_;
    std::vector<int> when_is_feeder_allowed_to_attack_;

    ActionList committed_actions_;
};

class CounterActionResult {
public:
    int ships_gained;
    ActionList defense_plan;
};


#endif
