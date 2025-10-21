// platform/platform_types.h
#pragma once

#include <cstdint>
#include <string>

namespace Core {

enum class Key : uint16_t {
    Unknown = 0,
    
    // Printable keys
    Space = 32,
    Apostrophe = 39,
    Comma = 44,
    Minus = 45,
    Period = 46,
    Slash = 47,
    
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    Semicolon = 59,
    Equal = 61,
    
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    GraveAccent = 96,
    
    // Function keys
    Escape = 256,
    Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
    
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
    
    // Keypad
    Kp0 = 320, Kp1, Kp2, Kp3, Kp4, Kp5, Kp6, Kp7, Kp8, Kp9,
    KpDecimal, KpDivide, KpMultiply, KpSubtract, KpAdd, KpEnter, KpEqual,
    
    // Modifiers
    LeftShift = 340,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu
};

enum class KeyAction : uint8_t {
    Release = 0,
    Press = 1,
    Repeat = 2
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4,
    Button6 = 5,
    Button7 = 6,
    Button8 = 7
};

enum class TouchPhase : uint8_t {
    Began = 0,
    Moved = 1,
    Ended = 2,
    Cancelled = 3
};

enum class AppState : uint8_t {
    Created = 0,
    Running = 1,
    Paused = 2,
    Stopped = 3,
    Destroying = 4
};

enum class FileLocation : uint8_t {
    Assets = 0,
    UserData = 1,
    Cache = 2,
    External = 3
};

namespace KeyMod {
    constexpr int Shift = 0x0001;
    constexpr int Control = 0x0002;
    constexpr int Alt = 0x0004;
    constexpr int Super = 0x0008;
    constexpr int CapsLock = 0x0010;
    constexpr int NumLock = 0x0020;
}

struct KeyEvent {
    Key key;
    int scancode;
    KeyAction action;
    int mods;
};

struct MouseButtonEvent {
    MouseButton button;
    KeyAction action;
    int mods;
    float x;
    float y;
};

struct MouseMoveEvent {
    float x;
    float y;
    float deltaX;
    float deltaY;
};

struct ScrollEvent {
    float xOffset;
    float yOffset;
};

struct TouchPoint {
    int32_t id;
    float x;
    float y;
    TouchPhase phase;
    float pressure;
    uint64_t timestamp;
};

struct TouchEvent {
    TouchPoint point;
};

struct FileInfo {
    std::string name;
    uint64_t size;
    uint64_t modifiedTime;
    bool isDirectory;
};

} // namespace Core
