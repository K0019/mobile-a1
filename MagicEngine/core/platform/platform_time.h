// platform/time.h
#pragma once

#include <cstdint>

namespace Core {

class Time {
public:
    static void Initialize();
    static uint64_t GetTicks();
    static double GetTickFrequency();
    static double GetSeconds();
    static double GetMilliseconds();
    static double TicksToSeconds(uint64_t ticks);
    static double TicksToMilliseconds(uint64_t ticks);
    static uint64_t GetUnixTimestamp();
    static void GetLocalTime(int& year, int& month, int& day, 
                            int& hour, int& minute, int& second);
    static void SleepMilliseconds(uint32_t ms);
    static void SleepMicroseconds(uint64_t us);

private:
    static uint64_t s_startTicks;
    static double s_ticksToSeconds;
};

} // namespace Core
