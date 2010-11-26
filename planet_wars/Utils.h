//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Switch between test environment and contest environment.
//#define IS_SUBMISSION

//Various switches in the bot's logic.
#define WITH_TIMEOUTS
//#define MAKE_ENEMY_MOVES_ON_FIRST_TURN
//#define STATIC_CONST_HORIZON

#ifndef IS_SUBMISSION
 #define CALCULATE_NUM_INVOKES_OF_RETURN_ON_MOVE
#endif

/***** Basic Mechanics *****/
#define RESERVE_ENEMY_ARRIVALS

/***** Return Calculation Settings *****/
//#define ADD_FUTURE_ENEMY_ARRIVALS_TO_SHIPS_SENT

#define ADD_SHIPS_LOST_TO_NEUTRALS
#define SUBTRACT_FUTURE_ATTACK_ARRIVALS_FROM_SHIPS_SENT
//#define USE_PARTIAL_POTENTIAL_UPDATES
#define USE_EXTENDED_POTENTIAL_GAINS
//#define USE_MAX_POTENTIAL_SHIPS_GAINED_FOR_EXTENDED_GAINS
#define CONSIDER_MY_EXTENDED_POTENTIAL_GAINS
#define USE_FULL_POTENTIALS_FOR_POTENTIAL_GAINS

//#define UPDATE_ADDITIONAL_GROWTH_TURNS
static const int kTurnsPerGame = 200;
static const int kAdditionalTurnsOfGrowth = 0;

/***** UpdatePotential Settings *****/
//#define USE_MAX_DEFENCE_POTENTIALS
//#define USE_MIN_SUPPORT_POTENTIALS
#define USE_TOTAL_SHIPS_FOR_POTENTIAL_FROM_SMALLER_ENEMIES
#define USE_INCREMENTAL_POTENTIAL_UPDATES

#ifndef IS_SUBMISSION
 //#define USE_SHADOW_TIMELINES
 //#define CALCULATE_FULL_POTENTIALS
#endif

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
#define USE_BETTER_CONSTRAINTS
#define USE_DEFENSE_POTENTIAL_FOR_CONSTRAINTS
#define ADD_EXCESS_SUPPORT_SHIPS
#define USE_DEFENSE_POTENTIAL_FOR_EXCESS_SUPPORT_CALCULATIONS
//#define CHECK_SUPPORT_DECLINE_ON_ALL_MY_PLANETS

/***** Feeder Planet Settings *****/
//#define USE_SHIPS_GAINED_TO_RESTRAIN_FEEDERS
#define DO_NOT_FEED_ON_FIRST_TURN
#define USE_IMPROVED_FEEDING

/***** Defense Through Offense ****/
#define FREE_SHIPS_ON_HOPELESS_PLANETS
const int kLatestPermanentLossForHopelessness = 10;

//Number of planets and planet flags.
#include <bitset>
const int kMaxNumPlanets = 30;
typedef std::bitset<kMaxNumPlanets> PlanetSelection;

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
