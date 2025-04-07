/******************************************************************************/
/*!
\file   Input.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/26/2024

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
	This is the cpp file for the game's input movement system.
	Functions and global static variables will be declared here.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Input.h"
#include "CustomViewport.h"


// static variables
std::bitset<GLFW_KEY_LAST + 1> Input::keystate, Input::pressedKeystate, Input::releasedKeystate;
Vector2 Input::mousePos = { 0.0f, 0.0f };
float Input::scrollOffset = 0.0f;
int Input::currIteration{};

GLFWgamepadstate GamepadInput::prevState{};
bool GamepadInput::gamepadActive{};


void Input::NewFrame()
{
	keystate &= ~releasedKeystate;

	pressedKeystate.reset();
	releasedKeystate.reset();

	scrollOffset = 0.0f;

	currIteration = GameTime::RealNumFixedFrames();
}

void Input::NewIteration()
{
	pressedKeystate.reset();
	--currIteration;
}

void Input::OnKeyDown(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
	{
		pressedKeystate[key] = true;
		keystate[key] = true;
	}
}

void Input::OnKeyUp(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
		releasedKeystate[key] = true;
}

void Input::OnMouseMove(double mouseX, double mouseY)
{
	mousePos.x = static_cast<float>(mouseX);
	mousePos.y = static_cast<float>(mouseY);
}

bool Input::GetKeyCurr(KEY key)
{
	return keystate[+key];
}

bool Input::GetKeyPressed(KEY key)
{
	return pressedKeystate[+key];
}

bool Input::GetKeyReleased(KEY key)
{
	// Only return true if we're at the last iteration.
	if (currIteration != 1)
		return false;
	return releasedKeystate[+key];
}

const Vector2& Input::GetMousePosRaw()
{
	return mousePos;
}

Vector2 Input::GetMousePosWorld()
{
#ifdef IMGUI_ENABLED
	return ST<CustomViewport>::Get()->WindowToWorldPosition(Vector2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y });
#else
	return ST<CustomViewport>::Get()->WindowToWorldPosition(Input::mousePos);
#endif

}

void Input::OnScroll(float offset)
{
	scrollOffset = offset;
}

float Input::GetScroll()
{
	return scrollOffset;
}

void GamepadInput::PollInput()
{
	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
	{
		gamepadActive = false;
		return;
	}

	GLFWgamepadstate state{};
	glfwGetGamepadState(GLFW_JOYSTICK_1, &state);

	auto ProcessButton{ [&state](int gamepadKeyId, short keyboardKeyId) -> void {
		if (state.buttons[gamepadKeyId] == prevState.buttons[gamepadKeyId])
			return;
		if (state.buttons[gamepadKeyId] == GLFW_PRESS)
		{
			gamepadActive = true;
			Input::OnKeyDown(keyboardKeyId);
		}
		else
			Input::OnKeyUp(keyboardKeyId);
	} };
	auto ProcessButtonAsScroll{ [&state](int gamepadKeyId, float scrollAmt) -> void {
		if (state.buttons[gamepadKeyId] == prevState.buttons[gamepadKeyId])
			return;
		if (state.buttons[gamepadKeyId] == GLFW_PRESS)
		{
			gamepadActive = true;
			Input::OnScroll(scrollAmt);
		}
	} };
	auto ProcessAxis{ [&state](int gamepadAxisId, KEY keyboardKeyLeft, KEY keyboardKeyRight) -> void {
		if (state.axes[gamepadAxisId] < -0.15f)
		{
			gamepadActive = true;
			if (Input::GetKeyCurr(keyboardKeyRight))
				Input::OnKeyUp(static_cast<short>(keyboardKeyRight));
			if (!Input::GetKeyCurr(keyboardKeyLeft))
				Input::OnKeyDown(static_cast<short>(keyboardKeyLeft));
		}
		else if (state.axes[gamepadAxisId] > 0.15f)
		{
			gamepadActive = true;
			if (Input::GetKeyCurr(keyboardKeyLeft))
				Input::OnKeyUp(static_cast<short>(keyboardKeyLeft));
			if (!Input::GetKeyCurr(keyboardKeyRight))
				Input::OnKeyDown(static_cast<short>(keyboardKeyRight));
		}
		else if (gamepadActive)
		{
			if (Input::GetKeyCurr(keyboardKeyLeft))
				Input::OnKeyUp(static_cast<short>(keyboardKeyLeft));
			if (Input::GetKeyCurr(keyboardKeyRight))
				Input::OnKeyUp(static_cast<short>(keyboardKeyRight));
		}
	} };
	auto ProcessTriggerAxis{ [&state](int gamepadAxisId, KEY keyboardKey) -> void {
		if (state.axes[gamepadAxisId] > 0.0f)
		{
			if (!Input::GetKeyCurr(keyboardKey))
				Input::OnKeyDown(static_cast<short>(keyboardKey));
		}
		else if (gamepadActive)
		{
			if (Input::GetKeyCurr(keyboardKey))
				Input::OnKeyUp(static_cast<short>(keyboardKey));
		}
	} };

	ProcessButton(GLFW_GAMEPAD_BUTTON_A, GLFW_KEY_SPACE);
	ProcessButton(GLFW_GAMEPAD_BUTTON_B, GLFW_KEY_R);
	ProcessButton(GLFW_GAMEPAD_BUTTON_X, GLFW_MOUSE_BUTTON_LEFT);
	ProcessButton(GLFW_GAMEPAD_BUTTON_Y, GLFW_KEY_F);
	ProcessButtonAsScroll(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 1.0f);
	ProcessButtonAsScroll(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, -1.0f);
	ProcessButton(GLFW_GAMEPAD_BUTTON_START, GLFW_KEY_ESCAPE);

	ProcessAxis(GLFW_GAMEPAD_AXIS_LEFT_X, KEY::A, KEY::D);
	ProcessTriggerAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, KEY::M_RIGHT);
	ProcessTriggerAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, KEY::M_LEFT);

	prevState = state;
}
