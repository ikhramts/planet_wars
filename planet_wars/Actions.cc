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

Action::Action() {
}

Action* Action::Get() {
    Action* action = s_pool_->GetAction();
    return action;
}

void Action::Free() {
    s_pool_->FreeAction(this);
}

Action::~Action() {
}

std::string Action::ToMoveOrder() const {
    pw_assert(NULL != source_);
    pw_assert(NULL != target_);
    
    std::stringstream move_order;

    if (0 == departure_time_) {
        move_order << source_->Id() << " " 
            << target_->Id() << " " << num_ships_;
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


