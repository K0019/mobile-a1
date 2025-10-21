// clock.cpp
#include "clock.h"

#include "core/platform/platform_time.h"

namespace Core {

Clock::Clock() noexcept {
    Reset();
}

void Clock::Reset() noexcept {
    m_startTicks = Time::GetTicks();
    m_previousTicks = m_startTicks;
    m_deltaTicks = 0;
}

void Clock::Tick() noexcept {
    const uint64_t currentTicks = Time::GetTicks();
    m_deltaTicks = currentTicks - m_previousTicks;
    m_previousTicks = currentTicks;
}

float Clock::GetDeltaTime() const noexcept {
    return static_cast<float>(Time::TicksToSeconds(m_deltaTicks));
}

float Clock::GetElapsedTime() const noexcept {
    const uint64_t currentTicks = Time::GetTicks();
    return static_cast<float>(Time::TicksToSeconds(currentTicks - m_startTicks));
}

uint64_t Clock::GetDeltaTimeTicks() const noexcept {
    return m_deltaTicks;
}

uint64_t Clock::GetElapsedTimeTicks() const noexcept {
    return Time::GetTicks() - m_startTicks;
}

} // namespace Core