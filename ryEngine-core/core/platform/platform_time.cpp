// platform/time.cpp
#include "platform_time.h"
#include <ctime>

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#else
    #include <time.h>
    #include <unistd.h>
#endif

namespace Core {

uint64_t Time::s_startTicks = 0;
double Time::s_ticksToSeconds = 0.0;

void Time::Initialize() {
#if PLATFORM_WINDOWS
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    s_ticksToSeconds = 1.0 / static_cast<double>(frequency.QuadPart);
    
    LARGE_INTEGER startTime;
    QueryPerformanceCounter(&startTime);
    s_startTicks = startTime.QuadPart;
#else
    timespec ts;
    #ifdef __APPLE__
        clock_gettime(CLOCK_MONOTONIC, &ts);
    #else
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    #endif
    s_startTicks = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
    s_ticksToSeconds = 1.0 / 1000000000.0;
#endif
}

uint64_t Time::GetTicks() {
#if PLATFORM_WINDOWS
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart - s_startTicks;
#else
    timespec ts;
    #ifdef __APPLE__
        clock_gettime(CLOCK_MONOTONIC, &ts);
    #else
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    #endif
    uint64_t now = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
    return now - s_startTicks;
#endif
}

double Time::GetTickFrequency() {
    return 1.0 / s_ticksToSeconds;
}

double Time::GetSeconds() {
    return static_cast<double>(GetTicks()) * s_ticksToSeconds;
}

double Time::GetMilliseconds() {
    return static_cast<double>(GetTicks()) * s_ticksToSeconds * 1000.0;
}

double Time::TicksToSeconds(uint64_t ticks) {
    return static_cast<double>(ticks) * s_ticksToSeconds;
}

double Time::TicksToMilliseconds(uint64_t ticks) {
    return static_cast<double>(ticks) * s_ticksToSeconds * 1000.0;
}

uint64_t Time::GetUnixTimestamp() {
    return static_cast<uint64_t>(std::time(nullptr));
}

void Time::GetLocalTime(int& year, int& month, int& day, 
                       int& hour, int& minute, int& second) {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    
    year = local->tm_year + 1900;
    month = local->tm_mon + 1;
    day = local->tm_mday;
    hour = local->tm_hour;
    minute = local->tm_min;
    second = local->tm_sec;
}

void Time::SleepMilliseconds(uint32_t ms) {
#if PLATFORM_WINDOWS
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void Time::SleepMicroseconds(uint64_t us) {
#if PLATFORM_WINDOWS
    // Windows Sleep is milliseconds only
    Sleep(static_cast<DWORD>(us / 1000));
#else
    usleep(static_cast<useconds_t>(us));
#endif
}

} // namespace Platform