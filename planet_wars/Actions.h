//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file deals with Actions, i.e. possible moves that can be made
//by either player.  Actions are maintained in an ActionPool; they
//should not be created or destroyed manually.

#ifndef PLANET_WARS_ACTIONS_H_
#define PLANET_WARS_ACTIONS_H_

#include <string>
#include <vector>

class Action;
class ActionPool;
class PlanetTimeline;

typedef std::vector<Action*> ActionList;

#ifndef PlanetTimelineList
typedef std::vector<PlanetTimeline*> PlanetTimelineList;
#endif

class Action {
    friend class ActionPool;

public:
    Action();
    
    //This function should be used to get actions.
    static Action* Get();
    
    //This function should be used to release the action.
    void Free();

    static void FreeActions(const ActionList& actions);
    static PlanetTimelineList Sources(const ActionList& actions);
    static PlanetTimelineList SourcesAndTargets(const ActionList& actions);
    
    int Owner() const                       {return owner_;}
    PlanetTimeline* Source() const        {return source_;}
    PlanetTimeline* Target() const        {return target_;}
    int Distance() const                    {return distance_;}
    int DepartureTime() const               {return departure_time_;}
    int NumShips() const                    {return num_ships_;}
    bool IsContingent() const               {return is_contingent_;}

    void SetOwner(int owner)                {owner_ = owner;}
    void SetSource(PlanetTimeline* s)     {source_ = s;}
    void SetTarget(PlanetTimeline* t)     {target_ = t;}
    void SetDistance(int d)                 {distance_ = d;}
    void SetDepartureTime(int t)            {departure_time_ = t;}
    void SetNumShips(int n)                 {num_ships_ = n;}
    void SetContingent(bool contingent)     {is_contingent_ = contingent;}

    //Convert an action to a move order understandable by the game engine.
    std::string ToMoveOrder() const;

private:
    //Disallow destruction.
    ~Action();

    static void SetActionPool(ActionPool* p)        {s_pool_ = p;}

    static ActionPool* s_pool_;

    int owner_;
    PlanetTimeline* source_;
    PlanetTimeline* target_;
    int distance_;
    int num_ships_;
    int departure_time_;        //in turns from now.
    bool is_contingent_;
};

//This class manages the pool of actions.
class ActionPool {
public:
    ActionPool();
    ~ActionPool();

   void FreeAction(Action* action);
   Action* GetAction();

private:
    ActionList free_actions_;
};

#endif
