//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Various switches in the bot's logic.
#define WITH_TIMEOUTS
#define ENEMY_RESERVES_SHIPS_AGAINST_ARRIVALS
//#define MAKE_ENEMY_MOVES_ON_FIRST_TURN
#define USE_SUPPORT_POTENTIALS_FOR_ATTACK
//#define USE_FULL_POTENTIALS_FOR_POTENTIAL_GAINS
//#define PRE_APPLY_SUPPORT_ACTIONS
//#define STATIC_CONST_HORIZON
//#define USE_SUPPORT_CONSTRAINTS
//#define USE_SEPARATE_GAIN_CALCULATION_FOR_NEUTRALS
//#define ADD_FUTURE_ENEMY_ARRIVALS_TO_SHIPS_SENT
//#define LOSE_SHIPS_ONLY_TO_NEUTRALS

//#define UPDATE_ADDITIONAL_GROWTH_TURNS

//Switch between test environment and contest environment.
//#define IS_SUBMISSION

static const int kTurnsPerGame = 200;

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

#endif
