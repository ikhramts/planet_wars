//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Switch between test environment and contest environment.
//#define IS_SUBMISSION

//Various switches in the bot's logic.
#define WITH_TIMEOUTS
#define ENEMY_RESERVES_SHIPS_AGAINST_ARRIVALS
//#define MAKE_ENEMY_MOVES_ON_FIRST_TURN
#define USE_SUPPORT_POTENTIALS_FOR_ATTACK
//#define STATIC_CONST_HORIZON

/***** Return Calculation Settings *****/
//#define USE_SEPARATE_GAIN_CALCULATION_FOR_NEUTRALS
//#define ADD_FUTURE_ENEMY_ARRIVALS_TO_SHIPS_SENT
#define USE_FULL_POTENTIALS_FOR_POTENTIAL_GAINS
//#define ADD_DEFENSE_SHIPS_TO_SHIPS_SENT

//#define LOSE_SHIPS_ONLY_TO_NEUTRALS
#define LOSE_ALL_SHIPS_SPENT_ON_NEUTRALS
#define SUBTRACT_FUTURE_ATTACK_ARRIVALS_FROM_SHIPS_SENT

#define USE_DEFENSE_POTENTIAL_FOR_GAINS
#define USE_PARTIAL_POTENTIAL_UPDATES

//#define UPDATE_ADDITIONAL_GROWTH_TURNS
static const int kTurnsPerGame = 200;

/***** UpdatePotential Settings *****/
#define CALCULATE_ALL_DEFENSE_POTENTIALS
//#define USE_MAX_DEFENCE_POTENTIALS
#define USE_TOTAL_SHIPS_FOR_POTENTIAL_FROM_SMALLER_ENEMIES

/***** Support Action Settings *****/
#define FORBID_TINY_LATE_SUPPORT_ACTIONS
static const int kMaxTinySupportAction = 1;
static const int kEarliestLateTinySupportAction = 2;

#define FORBID_SMALL_LATE_SUPPORT_ACTIONS
static const int kMaxSmallSupportAction = 5;
static const int kEarliestLateSupportAction = 5;

#define FORBID_MEDIUM_LATE_SUPPORT_ACTIONS
static const int kMaxMediumSupportAction = 10;
static const int kEarliestLateMediumSupportAction = 10;

/***** Support Settings *****/
#define PRE_APPLY_SUPPORT_ACTIONS
#define USE_SUPPORT_CONSTRAINTS
//#define USE_BETTER_CONSTRAINTS
#define ADD_EXCESS_SUPPORT_SHIPS
#define USE_DEFENSE_POTENTIAL_FOR_EXCESS_SUPPORT_CALCULATIONS


/***** Feeder Planet Settings *****/
#define USE_FEEDER_POTENTIAL_CHECK_FIX
//#define USE_SHIPS_GAINED_TO_RESTRAIN_FEEDERS
#define USE_FULL_POTENTIAL_FOR_SUPPORT_WORSENING
#define DO_NOT_FEED_ON_FIRST_TURN

//#define CONSIDER_ENEMY_MOVES_VS_NEUTRALS

//Define assertions.
#ifdef IS_SUBMISSION

    #define pw_assert(_expr) ((void)0)

    #ifndef WITH_TIMEOUTS
        #define WITH_TIMEOUTS
    #endif

#else

    #include <assert.h>
    #define pw_assert(_expr) assert(_expr)

#endif

//Useful definitions.
#ifndef uint
    typedef unsigned int uint;
#endif

#ifndef NULL
    #define NULL 0
#endif

//Useful functions
void forceCrash();

inline void SetMax(int& target, int other) {
    if (target < other) {
        target = other;
    }
}

inline void SetMin(int& target, int other) {
    if (target > other) {
        target = other;
    }
}

#endif
