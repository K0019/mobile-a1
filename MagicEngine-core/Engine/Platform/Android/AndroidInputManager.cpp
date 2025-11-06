#pragma once
#include "AndroidInputManager.h"

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }


AndroidInputComp::AndroidInputComp()
{
}

void AndroidInputComp::OnAttached()
{
    IGameComponentCallbacks::OnAttached();
}

void AndroidInputComp::OnStart()
{
    placeInitialCenter(); 
    m_center = Vec2{ 0.f,0.f };
    m_value  = Vec2{ 0.f,0.f };
    m_active = false;
}

void AndroidInputComp::OnDetached()
{
}

void AndroidInputComp::placeInitialCenter() {
    // default bottom-left with a small margin; adjustable as necessary
    m_center = Vec2{ 120.f, Core::Platform::Get().GetDisplay().GetHeight() - 120.f };
}


void AndroidInputComp::Update()
{
}

void AndroidInputComp::EditorDraw()
{
}
