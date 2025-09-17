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
	if (deviceType == INPUT_DEVICE_TYPE::INVALID_DEVICE || keyIdentifier < 0)
		return false;
	return GetValueFromDevice[+deviceType](keyIdentifier, readType);
}

INPUT_DEVICE_TYPE InputHardwareValueLink::GetDeviceType() const
{
	return deviceType;
}
void InputHardwareValueLink::SetDeviceType(INPUT_DEVICE_TYPE newDeviceType)
{
	keyIdentifier = -1;
	deviceType = newDeviceType;
}

int InputHardwareValueLink::GetKeyIdentifier() const
{
	return keyIdentifier;
}
void InputHardwareValueLink::SetKeyIdentifier(int newKeyIdentifier)
{
	keyIdentifier = newKeyIdentifier;
}

InputActionBase::InputActionBase(INPUT_COMPOSITE_TYPE compositeType)
	: compositeType{ compositeType }
{
}

INPUT_COMPOSITE_TYPE InputActionBase::GetCompositeType() const
{
	return compositeType;
}

bool InputSet::CreateNewAction(const std::string& name)
{
	return actions.try_emplace(name, std::make_shared<InputAction<bool>>()).second;
}

SPtr<const InputActionBase> InputSet::GetAction(const std::string& name) const
{
	auto actionIter{ actions.find(name) };
	if (actionIter == actions.end())
		return nullptr;
	return actionIter->second;
}
SPtr<InputActionBase> InputSet::GetAction(const std::string& name)
{
	auto actionIter{ actions.find(name) };
	if (actionIter == actions.end())
		return nullptr;
	return actionIter->second;
}

void InputSet::SetAction(const std::string& name, SPtr<InputActionBase>&& action)
{
	actions[name] = std::forward<SPtr<InputActionBase>>(action);
}

decltype(util::ToSortedVectorOfRefs(InputSet::actions)) InputSet::Editor_GetActions()
{
	return util::ToSortedVectorOfRefs(actions);
}

bool KeyboardMouseInput::GetIsPressed(KEY key) const
{
	return pressedKeystate[+key];
}

bool KeyboardMouseInput::GetIsReleased(KEY key) const
{
	if (ST<Input>::Get()->IsFinalIterationThisFrame())
		return false;
	return releasedKeystate[+key];
}

bool KeyboardMouseInput::GetIsDown(KEY key) const
{
	return keystate[+key];
}

bool KeyboardMouseInput::GetValue(INPUT_READ_TYPE readType, KEY key) const
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

float KeyboardMouseInput::GetScroll() const
{
	return scrollOffset;
}

void KeyboardMouseInput::NewFrame()
{
	keystate &= ~releasedKeystate;

	pressedKeystate.reset();
	releasedKeystate.reset();

	scrollOffset = 0.0f;
}

void KeyboardMouseInput::NewIteration()
{
	pressedKeystate.reset();
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

void Input::NewFrame()
{
	ST<KeyboardMouseInput>::Get()->NewFrame();

	currIteration = GameTime::RealNumFixedFrames();
}

void Input::NewIteration()
{
	ST<KeyboardMouseInput>::Get()->NewIteration();

	--currIteration;
}

bool Input::IsFinalIterationThisFrame() const
{
	return currIteration <= 1;
}

bool Input::CreateInputSet(const std::string& name)
{
	return inputSets.try_emplace(name, std::make_shared<InputSet>()).second;
}

bool Input::SwitchInputSet(const std::string& inputSetIdentifier)
{
	auto inputSetIter{ inputSets.find(inputSetIdentifier) };
	if (inputSetIter == inputSets.end())
		return false;

	currentInputSet = inputSetIter->second;
	return true;
}

SPtr<const InputSet> Input::GetCurrentInputSet() const
{
	return currentInputSet.lock();
}
SPtr<InputSet> Input::GetCurrentInputSet()
{
	return currentInputSet.lock();
}

decltype(util::ToSortedVectorOfRefs(Input::inputSets)) Input::Editor_GetInputSets()
{
	return util::ToSortedVectorOfRefs(inputSets);
}

WPtr<InputSet> Input::Editor_GetCurrentInputSet()
{
	return currentInputSet;
}


// static variables
//GLFWgamepadstate GamepadInput::prevState{};
//bool GamepadInput::gamepadActive{};
//
//
//void GamepadInput::PollInput()
//{
//	if (!glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
//	{
//		gamepadActive = false;
//		return;
//	}
//
//	GLFWgamepadstate state{};
//	glfwGetGamepadState(GLFW_JOYSTICK_1, &state);
//
//	auto ProcessButton{ [&state](int gamepadKeyId, short keyboardKeyId) -> void {
//		if (state.buttons[gamepadKeyId] == prevState.buttons[gamepadKeyId])
//			return;
//		if (state.buttons[gamepadKeyId] == GLFW_PRESS)
//		{
//			gamepadActive = true;
//			InputOld::OnKeyDown(keyboardKeyId);
//		}
//		else
//			InputOld::OnKeyUp(keyboardKeyId);
//	} };
//	auto ProcessButtonAsScroll{ [&state](int gamepadKeyId, float scrollAmt) -> void {
//		if (state.buttons[gamepadKeyId] == prevState.buttons[gamepadKeyId])
//			return;
//		if (state.buttons[gamepadKeyId] == GLFW_PRESS)
//		{
//			gamepadActive = true;
//			InputOld::OnScroll(scrollAmt);
//		}
//	} };
//	auto ProcessAxis{ [&state](int gamepadAxisId, KEY keyboardKeyLeft, KEY keyboardKeyRight) -> void {
//		if (state.axes[gamepadAxisId] < -0.15f)
//		{
//			gamepadActive = true;
//			if (InputOld::GetKeyCurr(keyboardKeyRight))
//				InputOld::OnKeyUp(static_cast<short>(keyboardKeyRight));
//			if (!InputOld::GetKeyCurr(keyboardKeyLeft))
//				InputOld::OnKeyDown(static_cast<short>(keyboardKeyLeft));
//		}
//		else if (state.axes[gamepadAxisId] > 0.15f)
//		{
//			gamepadActive = true;
//			if (InputOld::GetKeyCurr(keyboardKeyLeft))
//				InputOld::OnKeyUp(static_cast<short>(keyboardKeyLeft));
//			if (!InputOld::GetKeyCurr(keyboardKeyRight))
//				InputOld::OnKeyDown(static_cast<short>(keyboardKeyRight));
//		}
//		else if (gamepadActive)
//		{
//			if (InputOld::GetKeyCurr(keyboardKeyLeft))
//				InputOld::OnKeyUp(static_cast<short>(keyboardKeyLeft));
//			if (InputOld::GetKeyCurr(keyboardKeyRight))
//				InputOld::OnKeyUp(static_cast<short>(keyboardKeyRight));
//		}
//	} };
//	auto ProcessTriggerAxis{ [&state](int gamepadAxisId, KEY keyboardKey) -> void {
//		if (state.axes[gamepadAxisId] > 0.0f)
//		{
//			if (!InputOld::GetKeyCurr(keyboardKey))
//				InputOld::OnKeyDown(static_cast<short>(keyboardKey));
//		}
//		else if (gamepadActive)
//		{
//			if (InputOld::GetKeyCurr(keyboardKey))
//				InputOld::OnKeyUp(static_cast<short>(keyboardKey));
//		}
//	} };
//
//	ProcessButton(GLFW_GAMEPAD_BUTTON_A, GLFW_KEY_SPACE);
//	ProcessButton(GLFW_GAMEPAD_BUTTON_B, GLFW_KEY_R);
//	ProcessButton(GLFW_GAMEPAD_BUTTON_X, GLFW_MOUSE_BUTTON_LEFT);
//	ProcessButton(GLFW_GAMEPAD_BUTTON_Y, GLFW_KEY_F);
//	ProcessButtonAsScroll(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, 1.0f);
//	ProcessButtonAsScroll(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, -1.0f);
//	ProcessButton(GLFW_GAMEPAD_BUTTON_START, GLFW_KEY_ESCAPE);
//
//	ProcessAxis(GLFW_GAMEPAD_AXIS_LEFT_X, KEY::A, KEY::D);
//	ProcessTriggerAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, KEY::M_RIGHT);
//	ProcessTriggerAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, KEY::M_LEFT);
//
//	prevState = state;
//}
