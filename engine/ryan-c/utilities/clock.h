#pragma once
#include <chrono> // Required for steady_clock, duration, time_point

struct Time
{
  using clock = std::chrono::steady_clock;
  using time_point = clock::time_point;
  using duration = clock::duration;

  static time_point Now() noexcept;

  static std::int64_t NowMicroseconds() noexcept;
};

class Clock
{
  public:
    Clock() noexcept;

    void Reset() noexcept;

    void Tick() noexcept;

    // Gets the time elapsed since the previous call to Tick(), in seconds.
    // Commonly known as "delta time" or "dt".
    // Returns 0.0f on the very first call after Reset() or construction.
    [[nodiscard]] float GetDeltaTime() const noexcept;

    // Gets the total time elapsed since the Timer was constructed or last Reset(), in seconds.
    [[nodiscard]] float GetElapsedTime() const noexcept;

    [[nodiscard]] double GetDeltaTimeDouble() const noexcept;

    [[nodiscard]] double GetElapsedTimeDouble() const noexcept;

    [[nodiscard]] Time::clock::duration GetDeltaTimeDuration() const noexcept;

    [[nodiscard]] Time::clock::duration GetElapsedTimeDuration() const noexcept;

  private:
    Time::clock::time_point m_startTime;
    Time::clock::time_point m_previousTime;
    Time::clock::duration m_deltaTime{};
};
