//Available under GPLv3.
//Author: Iouri Khramtsov.

//This file deals with Actions, i.e. possible moves that can be made
//by either player.  Actions are maintained in an ActionPool; they
//should not be created or destroyed manually.

#include <sstream>

#include "Utils.h"
#include "Actions.h"
#include "GameTimeline.h"

/************************************************
               Action class
************************************************/
ActionPool* Action::s_pool_ = NULL;

Action::Action()
:is_contingent_(false) {
}

Action* Action::Get() {
    Action* action = s_pool_->GetAction();
    return action;
}

void Action::Free() {
    s_pool_->FreeAction(this);
}

void Action::FreeActions(const ActionList& actions) {
    for (uint i = 0; i < actions.size(); ++i) {
        actions[i]->Free();
    }
}

PlanetTimelineList Action::Sources(const ActionList &actions) {
    PlanetTimelineList sources;

    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        PlanetTimeline* source = action->Source();
        bool found_source = false;

        for (uint j = 0; j < sources.size(); ++j) {
            PlanetTimeline* planet = sources[j];
            
            if (source == planet) {
                found_source = true;
                break;
            }
        }

        if (!found_source) {
            sources.push_back(source);
        }
    }

    return sources;
}

PlanetSelection Action::SourcesAndTargets(const ActionList &actions) {
    PlanetSelection sources_and_targets;

    for (uint i = 0; i < actions.size(); ++i) {
        Action* action = actions[i];
        sources_and_targets[action->Source()->Id()] = true;
        sources_and_targets[action->Target()->Id()] = true;

        //PlanetTimeline* source = action->Source();
        //PlanetTimeline* target = action->Target();
        //bool found_source = false;
        //bool found_target = false;

        //for (uint j = 0; j < sources_and_targets.size(); ++j) {
        //    PlanetTimeline* planet = sources_and_targets[j];
        //    
        //    if (source == planet) {
        //        found_source = true;

        //    } else if (target == planet) {
        //        found_target = true;
        //    }
        //}

        //if (!found_source) {
        //    sources_and_targets.push_back(source);
        //}

        //if (!found_target) {
        //    sources_and_targets.push_back(target);
        //}
    }

    return sources_and_targets;
}

Action::~Action() {
}

std::string Action::ToMoveOrder() const {
    pw_assert(NULL != source_);
    pw_assert(NULL != target_);
    
    std::stringstream move_order;

    if (0 == departure_time_) {
        move_order << source_->Id() << " " 
            << target_->Id() << " " << num_ships_ << std::endl;
    }

    return move_order.str();
}

/************************************************
               ActionPool class
************************************************/
ActionPool::ActionPool() {
    Action::SetActionPool(this);

    //Initialize a few actions.
    free_actions_.resize(5000);

	for (uint i = 0; i < free_actions_.size(); ++i) {
		free_actions_[i] = new Action();
	}
}

ActionPool::~ActionPool() {
    for (uint i = 0; i < free_actions_.size(); ++i) {
        delete free_actions_[i];
    }
}

void ActionPool::FreeAction(Action *action) {
    action->SetContingent(false);
    free_actions_.push_back(action);
}

Action* ActionPool::GetAction() {
    Action* action = NULL;
    
    if (free_actions_.empty()) {
        action = new Action();
    
    } else {
        action = free_actions_.back();
        free_actions_.pop_back();
    }

    return action;
}


