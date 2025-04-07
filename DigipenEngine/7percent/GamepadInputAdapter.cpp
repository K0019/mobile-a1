/******************************************************************************/
/*!
\file   GamepadInputAdapter.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a gamepad input adapter system for this game.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "pch.h"
#include "GamepadInputAdapter.h"
#include "Engine.h"
#include "percentmath.h"

GamepadAimAdapterSystem::GamepadAimAdapterSystem()
	: System_Internal{ &GamepadAimAdapterSystem::UpdateComp }
{
}

void GamepadAimAdapterSystem::UpdateComp(GamepadAimAdapterComponent& comp)
{
	// Don't run this system while in pause menu
	if (GameTime::GetTimeScale() == 0.0f)
		return;
	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
		return;

	GLFWgamepadstate gamepadState{};
	glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepadState);
	Vector2 aimVector{ gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] };
	// Deadzone check
	if (aimVector.LengthSquared() <= 0.15f * 0.15f)
		return;

	// Calculate the position of the mouse in window coordinates
	Transform windowTransform{ ST<CustomViewport>::Get()->WorldToWindowTransform(ecs::GetEntityTransform(&comp)) };
	windowTransform.AddLocalPosition(aimVector.Normalize() * 300.0f);
	Vector2 mousePos{ windowTransform.GetLocalPosition() };

	// Clamp mouse position to window bounds
	mousePos.x = math::Clamp(mousePos.x, 0.0f, static_cast<float>(ST<Engine>::Get()->_windowExtent.width));
	mousePos.y = math::Clamp(mousePos.y, 0.0f, static_cast<float>(ST<Engine>::Get()->_windowExtent.height));
	glfwSetCursorPos(ST<Engine>::Get()->_window, mousePos.x, mousePos.y);
	Input::OnMouseMove(mousePos.x, mousePos.y);
}

GamepadMouseControlSystem::GamepadMouseControlSystem()
	: System_Internal{ &GamepadMouseControlSystem::UpdateComp }
{
}

void GamepadMouseControlSystem::UpdateComp(GamepadMouseControlComponent&)
{
	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
		return;

	GLFWgamepadstate gamepadState{};
	glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepadState);
	Vector2 aimVector{ gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X], gamepadState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] };
	// Deadzone check
	if (aimVector.LengthSquared() <= 0.15f * 0.15f)
		return;

	double x{}, y{};
	glfwGetCursorPos(ST<Engine>::Get()->_window, &x, &y);
	Vector2 mousePos{ static_cast<float>(x), static_cast<float>(y) };
	mousePos += aimVector.Normalize() * 7.0f;
	// Clamp mouse position to window bounds
	mousePos.x = math::Clamp(mousePos.x, 0.0f, static_cast<float>(ST<Engine>::Get()->_windowExtent.width));
	mousePos.y = math::Clamp(mousePos.y, 0.0f, static_cast<float>(ST<Engine>::Get()->_windowExtent.height));
	glfwSetCursorPos(ST<Engine>::Get()->_window, mousePos.x, mousePos.y);
	Input::OnMouseMove(mousePos.x, mousePos.y);
}
