#pragma once

#if defined(TRACY_ENABLE)
#include "tracy/Tracy.hpp"
// predefined RGB colors for "heavy" point-of-interest operations
#define PROFILER_COLOR_WAIT 0xff0000
#define PROFILER_COLOR_SUBMIT 0x0000ff
#define PROFILER_COLOR_PRESENT 0x00ff00
#define PROFILER_COLOR_CREATE 0xff6600
#define PROFILER_COLOR_DESTROY 0xffa500
#define PROFILER_COLOR_BARRIER 0xffffff
#define PROFILER_COLOR_CMD_DRAW 0x8b0000
#define PROFILER_COLOR_CMD_COPY 0x8b0a50
#define PROFILER_COLOR_CMD_RTX 0x8b0000
#define PROFILER_COLOR_CMD_DISPATCH 0x8b0000
//
#define PROFILER_COLOR_CPU_UPDATE 0x32CD32
#define PROFILER_COLOR_CPU_RENDER_PREP 0x00BFFF
#define PROFILER_COLOR_CPU_PHYSICS 0xFF4500
#define PROFILER_COLOR_CPU_ANIMATION 0x4682B4
#define PROFILER_COLOR_CPU_LOGIC 0xBA55D3
#define PROFILER_COLOR_CPU_IO 0x808080
#define PROFILER_COLOR_CPU_OTHER 0xA52A2A
#define PROFILER_COLOR_CPU_SCRIPTING 0x8B008B

//
#define PROFILER_FUNCTION() ZoneScoped
#define PROFILER_FUNCTION_COLOR(color) ZoneScopedC(color)
#define PROFILER_ZONE(name, color) \
    {                                    \
      ZoneScopedC(color);                \
      ZoneName(name, strlen(name))
#define PROFILER_ZONE_END() }
#define PROFILER_THREAD(name) tracy::SetThreadName(name)
#define PROFILER_FRAME(name) FrameMarkNamed(name)
#else
#define PROFILER_FUNCTION()
#define PROFILER_FUNCTION_COLOR(color)
#define PROFILER_ZONE(name, color) {
#define PROFILER_ZONE_END() }
#define PROFILER_THREAD(name)
#define PROFILER_FRAME(name)
#endif // TRACY_ENABLE
