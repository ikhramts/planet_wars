//Available under GPLv3.
//Author: Iouri Khramtsov.

//General-purpose utilities.

#ifndef PLANET_WARS_UTILS_H_
#define PLANET_WARS_UTILS_H_

//Switch between test environment and contest environment.
#define IS_SUBMISSION

//Define assertions.
#ifdef IS_SUBMISSION

#define pw_assert(_expr) ((void)0)

#else

#include <assert.h>
#define pw_assert(_expr) assert(_expr)

#endif //#ifndef IS_SUBMISSION

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
