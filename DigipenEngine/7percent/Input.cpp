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

namespace internal {

	const std::array<InputHardwareValueLink::FuncType_GetValue, +INPUT_DEVICE_TYPE::NUM_DEVICES> InputHardwareValueLink::GetValueFromDevice{
		[](int keyIdentifier, INPUT_READ_TYPE readType) -> InputHardwareValue {
			return ST<KeyboardMouseInput>::Get()->GetValue(readType, static_cast<KEY>(keyIdentifier));
		}
	};


	InputHardwareValueLink::InputHardwareValueLink()
		: deviceType{ INPUT_DEVICE_TYPE::INVALID_DEVICE }
		, readType{ INPUT_READ_TYPE::CURRENT }
		, keyIdentifier{ -1 }
	{
	}

	InputHardwareValueLink::InputHardwareValueLink(INPUT_DEVICE_TYPE deviceType, int keyIdentifier, INPUT_READ_TYPE readType)
		: deviceType{ deviceType }
		, readType{ readType }
		, keyIdentifier{ keyIdentifier }
	{
	}

	InputHardwareValue InputHardwareValueLink::ReadValue() const
	{
		if (deviceType == INPUT_DEVICE_TYPE::INVALID_DEVICE)
			return false;
		return GetValueFromDevice[+deviceType](keyIdentifier, readType);
	}

}

bool KeyboardMouseInput::GetIsPressed(KEY key)
{
	return pressedKeystate[+key];
}

bool KeyboardMouseInput::GetIsReleased(KEY key)
{
	// TODO: Only return true if we're at the last iteration.
	//if (currIteration != 1)
	//	return false;
	return releasedKeystate[+key];
}

bool KeyboardMouseInput::GetIsDown(KEY key)
{
	return keystate[+key];
}

bool KeyboardMouseInput::GetValue(INPUT_READ_TYPE readType, KEY key)
{
	switch (readType)
	{
	case INPUT_READ_TYPE::PRESSED:
		return ST<KeyboardMouseInput>::Get()->GetIsPressed(key);
	case INPUT_READ_TYPE::RELEASED:
		return ST<KeyboardMouseInput>::Get()->GetIsReleased(key);
	case INPUT_READ_TYPE::CURRENT:
		return ST<KeyboardMouseInput>::Get()->GetIsDown(key);
	default:
		// Unimplemented INPUT_READ_TYPE
		assert(false);
		return false;
	}
}

void KeyboardMouseInput::GLFW_Callback_OnKeyboardClick([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mode)
{
	switch (action)
	{
	case GLFW_PRESS:
		ST<KeyboardMouseInput>::Get()->Callback_OnKeyDown(static_cast<short>(key));
		break;
	case GLFW_RELEASE:
		ST<KeyboardMouseInput>::Get()->Callback_OnKeyUp(static_cast<short>(key));
		break;
	}
}

void KeyboardMouseInput::GLFW_Callback_OnMouseClick([[maybe_unused]] GLFWwindow* window, int button, int action, [[maybe_unused]] int mode)
{
	switch (action)
	{
	case GLFW_PRESS:
		ST<KeyboardMouseInput>::Get()->Callback_OnKeyDown(static_cast<short>(button));
		break;
	case GLFW_RELEASE:
		ST<KeyboardMouseInput>::Get()->Callback_OnKeyUp(static_cast<short>(button));
		break;
	}
}

void KeyboardMouseInput::GLFW_Callback_OnMouseMove([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xpos, [[maybe_unused]] double ypos)
{
#ifdef IMGUI_ENABLED
	auto pos = ImGui::GetMousePos();
	ST<KeyboardMouseInput>::Get()->Callback_OnMouseMove(pos.x, pos.y);
#else
	// Clamp mouse position to window bounds
	double clampedXpos = math::Clamp(xpos, 0.0, static_cast<double>(ST<Engine>::Get()->_windowExtent.width));
	double clampedYpos = math::Clamp(ypos, 0.0, static_cast<double>(ST<Engine>::Get()->_windowExtent.height));
	if (clampedXpos != xpos || clampedYpos != ypos)
		glfwSetCursorPos(window, clampedXpos, clampedYpos);

	ST<KeyboardMouseInput>::Get()->Callback_OnMouseMove(clampedXpos, clampedYpos);
#endif
}

void KeyboardMouseInput::GLFW_Callback_OnMouseScroll([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xoffset, double yoffset)
{
	ST<KeyboardMouseInput>::Get()->Callback_OnMouseScroll(static_cast<float>(yoffset));
}

void KeyboardMouseInput::Callback_OnKeyDown(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
	{
		pressedKeystate[key] = true;
		keystate[key] = true;
	}
}
void KeyboardMouseInput::Callback_OnKeyUp(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
		releasedKeystate[key] = true;
}

void KeyboardMouseInput::Callback_OnMouseMove(double x, double y)
{
	mousePos.x = static_cast<float>(x);
	mousePos.y = static_cast<float>(y);
}

void KeyboardMouseInput::Callback_OnMouseScroll(float offset)
{
	scrollOffset = offset;
}


// static variables
std::bitset<GLFW_KEY_LAST + 1> InputOld::keystate, InputOld::pressedKeystate, InputOld::releasedKeystate;
Vec2 InputOld::mousePos = { 0.0f, 0.0f };
float InputOld::scrollOffset = 0.0f;
int InputOld::currIteration{};

GLFWgamepadstate GamepadInput::prevState{};
bool GamepadInput::gamepadActive{};


void InputOld::NewFrame()
{
	keystate &= ~releasedKeystate;

	pressedKeystate.reset();
	releasedKeystate.reset();

	scrollOffset = 0.0f;

	currIteration = GameTime::RealNumFixedFrames();
}

void InputOld::NewIteration()
{
	pressedKeystate.reset();
	--currIteration;
}

void InputOld::OnKeyDown(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
	{
		pressedKeystate[key] = true;
		keystate[key] = true;
	}
}

void InputOld::OnKeyUp(short key)
{
	// Certain special keys such as function key send a -1 keycode. Need to guard against those to avoid crashing
	if (key >= 0)
		releasedKeystate[key] = true;
}

void InputOld::OnMouseMove(double mouseX, double mouseY)
{
	mousePos.x = static_cast<float>(mouseX);
	mousePos.y = static_cast<float>(mouseY);
}

bool InputOld::GetKeyCurr(KEY key)
{
	return keystate[+key];
}

bool InputOld::GetKeyPressed(KEY key)
{
	return pressedKeystate[+key];
}

bool InputOld::GetKeyReleased(KEY key)
{
	// Only return true if we're at the last iteration.
	if (currIteration != 1)
		return false;
	return releasedKeystate[+key];
}

const Vec2& InputOld::GetMousePosRaw()
{
	return mousePos;
}

Vec3 InputOld::GetMousePosWorld()
{
#ifdef IMGUI_ENABLED
	return ST<CustomViewport>::Get()->WindowToWorldPosition(Vec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y });
#else
	return ST<CustomViewport>::Get()->WindowToWorldPosition(InputOld::mousePos);
#endif

}

void InputOld::OnScroll(float offset)
{
	scrollOffset = offset;
}

float InputOld::GetScroll()
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
			InputOld::OnKeyDown(keyboardKeyId);
		}
		else
			InputOld::OnKeyUp(keyboardKeyId);
	} };
	auto ProcessButtonAsScroll{ [&state](int gamepadKeyId, float scrollAmt) -> void {
		if (state.buttons[gamepadKeyId] == prevState.buttons[gamepadKeyId])
			return;
		if (state.buttons[gamepadKeyId] == GLFW_PRESS)
		{
			gamepadActive = true;
			InputOld::OnScroll(scrollAmt);
		}
	} };
	auto ProcessAxis{ [&state](int gamepadAxisId, KEY keyboardKeyLeft, KEY keyboardKeyRight) -> void {
		if (state.axes[gamepadAxisId] < -0.15f)
		{
			gamepadActive = true;
			if (InputOld::GetKeyCurr(keyboardKeyRight))
				InputOld::OnKeyUp(static_cast<short>(keyboardKeyRight));
			if (!InputOld::GetKeyCurr(keyboardKeyLeft))
				InputOld::OnKeyDown(static_cast<short>(keyboardKeyLeft));
		}
		else if (state.axes[gamepadAxisId] > 0.15f)
		{
			gamepadActive = true;
			if (InputOld::GetKeyCurr(keyboardKeyLeft))
				InputOld::OnKeyUp(static_cast<short>(keyboardKeyLeft));
			if (!InputOld::GetKeyCurr(keyboardKeyRight))
				InputOld::OnKeyDown(static_cast<short>(keyboardKeyRight));
		}
		else if (gamepadActive)
		{
			if (InputOld::GetKeyCurr(keyboardKeyLeft))
				InputOld::OnKeyUp(static_cast<short>(keyboardKeyLeft));
			if (InputOld::GetKeyCurr(keyboardKeyRight))
				InputOld::OnKeyUp(static_cast<short>(keyboardKeyRight));
		}
	} };
	auto ProcessTriggerAxis{ [&state](int gamepadAxisId, KEY keyboardKey) -> void {
		if (state.axes[gamepadAxisId] > 0.0f)
		{
			if (!InputOld::GetKeyCurr(keyboardKey))
				InputOld::OnKeyDown(static_cast<short>(keyboardKey));
		}
		else if (gamepadActive)
		{
			if (InputOld::GetKeyCurr(keyboardKey))
				InputOld::OnKeyUp(static_cast<short>(keyboardKey));
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

bool Input::CreateInputSet(const std::string& name)
{
	return inputSets.try_emplace(name).second;
}

bool Input::SwitchInputSet(const std::string& inputSetIdentifier)
{
	auto inputSetIter{ inputSets.find(inputSetIdentifier) };
	if (inputSetIter == inputSets.end())
		return false;

	currentInputSet = inputSetIter->second;
	return true;
}

SPtr<const internal::InputSet> Input::GetCurrentInputSet() const
{
	return currentInputSet.lock();
}
