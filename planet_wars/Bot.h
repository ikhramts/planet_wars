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
class SupportConstraints;

class Bot {
public:
    static const double kAggressionReturnMultiplier;
    static const int kSupportShipLimit = 40;

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
        int player);
    double ReturnForMove(const ActionList& invasion_plan, double best_return);
    double ReturnForMove2(ActionList& invasion_plan, double best_return, int depth);

    CounterActionResult ShipsGainedForAfterMove(const ActionList& invasion_plan, 
                                         PlanetTimelineList& counter_targets,
                                         const int attack_arrival_time);

    ActionList SendFleetsToFront(int player);

    //Mark feeder planets as such.
    void MarkReinforcers(int player);

    ActionList SendSupportFleets(int player);

    //Apply the actions to the timeline, and do some record-keeping while we're at it.
    void ApplyActions(const ActionList& actions);

    //Check which planets lost support due to the actions, and add appropriate
    //prohibitions on sending support fleets to those planets.
    void AddSupportConstraints(const ActionList& actions);
    
    GameMap* game_;
    GameTimeline* timeline_;
    int counter_horizon_;
    int defense_horizon_;
    int turn_;
    int picking_round_;
    std::vector<int> when_is_feeder_allowed_to_attack_;

    ActionList committed_actions_;
    SupportConstraints* support_constraints_;
};

class CounterActionResult {
public:
    int ships_gained;
    ActionList defense_plan;
};

/************************************************
            SupportConstraints class
************************************************/

class SupportConstraints {
public:
    SupportConstraints(int num_planets, GameMap* game);
    void AddConstraint(PlanetTimeline* constrained_planet, PlanetTimeline* constraint_center);
    bool MaySupport(PlanetTimeline* source, PlanetTimeline* target);
    void ClearConstraints();

private:
    std::vector<std::vector<int> > constraint_centers_;
    std::vector<std::vector<int> > constraint_radii_;
    GameMap* game_;
};

inline double ReturnRatio(int ships_gained, int ships_sent) {
    const double return_ratio = static_cast<double>(ships_gained)/static_cast<double>(ships_sent);
    return return_ratio;
}

#endif
