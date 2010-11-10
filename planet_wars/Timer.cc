#include "Timer.h"
#include "Utils.h"

/*
 * Two separate implementations.  A Linux implementation uses gettimeofday(),
 * which is the actual method of timing the contest entry.  Unfortunately, 
 * that is not available on Windows, so for testing on my computer I use clock().
 */

const long int SECONDS_PER_DAY = 86400;

#ifndef IS_SUBMISSION
    //We're on Windows.
    //Create our own version of gettimeofday().
    #include <time.h>
    #include <windows.h> //I've ommited this line.
    #if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
    #else
      #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
    #endif
    
    //Disable CRT security warnings.
    //#ifndef _CRT_SECURE_NO_WARNINGS
      #define _CRT_SECURE_NO_WARNINGS
    //#endif
     
    struct timezone 
    {
      int  tz_minuteswest; /* minutes W of Greenwich */
      int  tz_dsttime;     /* type of dst correction */
    };
     
    int gettimeofday(struct timeval *tv, struct timezone *tz)
    {
      FILETIME ft;
      unsigned __int64 tmpres = 0;
      static int tzflag;
     
      if (NULL != tv)
      {
        GetSystemTimeAsFileTime(&ft);
     
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;
     
        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS; 
        tmpres /= 10;  /*convert into microseconds*/
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
      }
     
      //if (NULL != tz)
      //{
      //  if (!tzflag)
      //  {
      //    _tzset();
      //    tzflag++;
      //  }
      //  tz->tz_minuteswest = _timezone / 60;
      //  tz->tz_dsttime = _daylight;
      //}
     
      return 0;
    }
#else
	#include <sys/time.h>
#endif

//In microseconds
long int gTimeOut = 0;
long int gStartTime = 0;

void SetTimeOut(double seconds) {
	timeval startTime;
	gettimeofday(&startTime, NULL);

	gStartTime = (startTime.tv_sec % SECONDS_PER_DAY)* 1000 + startTime.tv_usec / 1000;
	gTimeOut = static_cast<long int>(seconds * 1000.0);
}

bool HasTimedOut() {
	timeval currentTime;
	gettimeofday(&currentTime, NULL);

	long int currentMilliseconds = (currentTime.tv_sec % SECONDS_PER_DAY) * 1000 + currentTime.tv_usec / 1000;

	return ((currentMilliseconds - gStartTime) > gTimeOut);
}

int MillisElapsed() {
	timeval currentTime;
	gettimeofday(&currentTime, NULL);

	const long int currentMilliseconds = (currentTime.tv_sec % SECONDS_PER_DAY) * 1000 + currentTime.tv_usec / 1000;
    const long int millisElapsed = (currentMilliseconds - gStartTime);
    return millisElapsed;
}
