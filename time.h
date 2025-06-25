#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

uint64_t get_millis_timestamp()
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli = { .LowPart = ft.dwLowDateTime, .HighPart = ft.dwHighDateTime };
    return (uli.QuadPart / 10000LL) - 11644473600000LL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

#endif