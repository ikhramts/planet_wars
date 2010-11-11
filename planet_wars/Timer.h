/* 
Timer.  Measures out the time in milliseconds using the wall clock. 
Provides facilities for checking whether the timer has run out, as well
as the current elapsed time in milliseconds.

The timer does not use any signals, and will not notify of the timeout.
*/

#ifndef PLANET_WARS_TIMER_H_
#define PLANET_WARS_TIMER_H_

/*
Set the timeout for the timer and start the timer.  
Example: SetTimeOut(0.995) sets the timer for 995ms
*/
void SetTimeOut(double seconds);

/*
Check whether the timer has run out.
*/
bool HasTimedOut();

/*
Get the number of milliseconds since the last call to SetTimeOut()
*/
int MillisElapsed();

#ifndef NULL
#define NULL 0
#endif

#endif
