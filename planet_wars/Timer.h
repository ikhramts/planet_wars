/* Timer. */

#ifndef PLANET_WARS_TIMER_H_
#define PLANET_WARS_TIMER_H_

//When uncommented, indicates whether the code is being compiled for
//my own computer (a Windows machine).  
//#define TEST_ENVIRONMENT

//Keeping track of time.
void SetTimeOut(double seconds);
bool HasTimedOut();
int MillisElapsed();

#ifndef NULL
#define NULL 0
#endif

#endif /* TIMER_H_ */
