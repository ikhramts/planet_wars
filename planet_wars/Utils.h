//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Switch between test environment and contest environment.
#define IS_SUBMISSION

#ifndef IS_SUBMISSION
#include <assert.h>

#else

#ifndef assert
#define assert(_Expression)     ((void)0)
#endif //#ifndef assert

#endif //#ifndef IS_SUBMISSION

//Useful definitions.
#ifndef uint
typedef unsigned int uint;
#endif

#ifndef NULL
#define NULL 0
#endif

void forceCrash();

#endif
