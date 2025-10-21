// clock.h
#pragma once
#include <cstdint>

namespace Core {

class Clock {
public:
    Clock() noexcept;
    
    void Reset() noexcept;
    void Tick() noexcept;
    
    // Delta time in seconds since last Tick() - returns 0.0f on first call after Reset()
    [[nodiscard]] float GetDeltaTime() const noexcept;
    
    // Total elapsed time in seconds since construction or Reset()
    [[nodiscard]] float GetElapsedTime() const noexcept;
    
    // Raw tick values for when you need maximum precision
    [[nodiscard]] uint64_t GetDeltaTimeTicks() const noexcept;
    [[nodiscard]] uint64_t GetElapsedTimeTicks() const noexcept;

private:
    uint64_t m_startTicks;
    uint64_t m_previousTicks;
    uint64_t m_deltaTicks;
};

} // namespace Core