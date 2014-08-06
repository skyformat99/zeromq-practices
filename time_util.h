/**
 * @file time_util.h
 *
 * @breif Provide a portable implementations for some commonly used functions,
 * such as, msleep, mtime, etc.
 */
#ifndef _TIME_UTIL_H
#define _TIME_UTIL_H

#ifdef WIN32
# include <windows.h>
# include <time.h>
#else // Unix/Linux
# include <unistd.h>
# include <sys/time.h>
#endif // WIN32
#include <stdint.h>

// msleep - sleep for at least the specified milliseconds
inline void msleep(uint64_t ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    sleep(ms * 1000);
#endif
}

// mclock - high-resolution clock, return current UTC time in milliseconds
static uint64_t mclock(void)
{
#ifdef WIN32
    SYSTEMTIME st;
    GetSystemTime (&st);
    return (uint64_t) st.wSecond * 1000 + st.wMilliseconds;
#else
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (uint64_t) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

#endif // _TIME_UTIL_H
