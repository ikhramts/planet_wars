//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Various switches in the bot's logic.
//#define WITH_TIMEOUTS
#define ENEMY_RESERVES_SHIPS_AGAINST_ARRIVALS
//#define MAKE_ENEMY_MOVES_ON_FIRST_TURN
//#define USE_SEPARATE_SUPPORT_PLANS

//Switch between test environment and contest environment.
//#define IS_SUBMISSION

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
