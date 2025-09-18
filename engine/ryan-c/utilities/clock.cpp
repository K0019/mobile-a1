#include "clock.h"

Time::time_point Time::Now() noexcept { return clock::now(); }

std::int64_t Time::NowMicroseconds() noexcept
{
  return std::chrono::duration_cast<std::chrono::microseconds>(Now().time_since_epoch()).count();
}

Clock::Clock() noexcept { Reset(); }

void Clock::Reset() noexcept
{
  m_startTime = Time::Now();
  m_previousTime = m_startTime;
  m_deltaTime = Time::clock::duration::zero(); // Initialize delta time to zero
}

void Clock::Tick() noexcept
{
  const auto currentTime = Time::Now();
  m_deltaTime = currentTime - m_previousTime; // Calculate duration since last tick
  m_previousTime = currentTime;               // Update previous time for the *next* tick calculation
}

float Clock::GetDeltaTime() const noexcept
{
  // Convert the stored duration to seconds represented by a float
  return std::chrono::duration<float>(m_deltaTime).count();
}

float Clock::GetElapsedTime() const noexcept
{
  // Calculate duration from start time to the current moment
  const auto currentTime = Time::Now();
  return std::chrono::duration<float>(currentTime - m_startTime).count();
}

double Clock::GetDeltaTimeDouble() const noexcept { return std::chrono::duration<double>(m_deltaTime).count(); }

double Clock::GetElapsedTimeDouble() const noexcept
{
  const auto currentTime = Time::Now();
  return std::chrono::duration<double>(currentTime - m_startTime).count();
}

std::chrono::steady_clock::duration Clock::GetDeltaTimeDuration() const noexcept { return m_deltaTime; }

std::chrono::steady_clock::duration Clock::GetElapsedTimeDuration() const noexcept
{
  const auto currentTime = Time::Now();
  return currentTime - m_startTime;
}
