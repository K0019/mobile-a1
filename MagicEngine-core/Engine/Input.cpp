/******************************************************************************/
/*!
\file   Input.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/17/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Contains an interface to receive hardware input and converts it into an
	interface for game systems to read as an input action.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Input.h"
#include "core/platform/platform.h"

#ifdef IMGUI_ENABLED
#include <ImGui/ImguiHeader.h>
#else
#include "Engine/Graphics Interface/GraphicsWindow.h"
#endif

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

void InputHardwareValueLink::Serialize(Serializer& writer) const
{
	writer.Serialize("deviceType", +deviceType);
	writer.Serialize("keyIdentifier", +keyIdentifier);
}
void InputHardwareValueLink::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("deviceType", &deviceType);
	reader.DeserializeVar("keyIdentifier", &keyIdentifier);
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

bool InputSet::RenameAction(const std::string& oldName, const std::string& newName)
{
	if (actions.find(newName) != actions.end() || actions.find(oldName) == actions.end())
		return false;

	actions.try_emplace(newName, actions.at(oldName));
	actions.erase(oldName);
	return true;
}

void InputSet::Serialize(Serializer& writer) const
{
	writer.Serialize("actions", actions);
}

void InputSet::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("actions", &actions, [](Deserializer& inReader, decltype(actions)* map) -> void {
		std::pair<std::string, SPtr<InputActionBase>> pair{};
		inReader.DeserializeVar("key", &pair.first);

		// Need to push accesses downwards to find what type of action to initialize
		int actionType{};
		inReader.PushAccess("value");
		inReader.DeserializeVar("compositeType", &actionType);

		switch (static_cast<INPUT_COMPOSITE_TYPE>(actionType))
		{
#define X(name, str, type) \
		case INPUT_COMPOSITE_TYPE::name: \
			pair.second = std::make_shared<InputAction<type>>(); \
			inReader.DeserializeVar("", pair.second.get()); \
			break;
		ENUM_INPUT_COMPOSITE_TYPE
#undef X
		default:
			// Composite type doesn't exist. Ignore this entry.
			inReader.PopAccess();
			return;
		}

		map->emplace(std::move(pair));
		inReader.PopAccess();
	});
}

decltype(util::ToSortedVectorOfRefs(InputSet::actions)) InputSet::Editor_GetActions()
{
	return util::ToSortedVectorOfRefs(actions);
}

bool KeyboardMouseInput::GetIsPressed(KEY key) const
{
	//return pressedKeystate[+key];
	return Input::GetKeyDown(static_cast<Key>(key));
}

bool KeyboardMouseInput::GetIsReleased(KEY key) const
{
	if (ST<MagicInput>::Get()->IsFinalIterationThisFrame())
		return false;
	//return releasedKeystate[+key];
	return Input::GetKeyUp(static_cast<Key>(key));
}

bool KeyboardMouseInput::GetIsDown(KEY key) const
{
	//return keystate[+key];
	switch (key)
	{
	case KEY::M1: case KEY::M2: case KEY::M3:
		return Input::GetMouseButton(static_cast<MouseButton>(+key - +KEY::M1));
	default:
		return Input::GetKey(static_cast<Key>(key));
	}
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
	//return scrollOffset;
	return Input::GetScrollDelta().y;
}


Vec2 KeyboardMouseInput::GetMousePos() const
{
	//return mousePos;
	return Input::GetMousePosition();
}

Vec2 KeyboardMouseInput::GetMouseDelta() const {
	return Input::GetMouseDelta();
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

#ifdef GLFW
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
	auto windowExtent{ ST<GraphicsWindow>::Get()->GetWindowExtent() };
	double clampedXpos = std::clamp(xpos, 0.0, static_cast<double>(windowExtent.x));
	double clampedYpos = std::clamp(ypos, 0.0, static_cast<double>(windowExtent.y));
	if (clampedXpos != xpos || clampedYpos != ypos)
		glfwSetCursorPos(window, clampedXpos, clampedYpos);

	ST<KeyboardMouseInput>::Get()->Callback_OnMouseMove(clampedXpos, clampedYpos);
#endif
}

void KeyboardMouseInput::GLFW_Callback_OnMouseScroll([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xoffset, double yoffset)
{
	ST<KeyboardMouseInput>::Get()->Callback_OnMouseScroll(static_cast<float>(yoffset));
}
#endif

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

void MagicInput::NewFrame()
{
	ST<KeyboardMouseInput>::Get()->NewFrame();

	currIteration = GameTime::RealNumFixedFrames();
}

void MagicInput::NewIteration()
{
	ST<KeyboardMouseInput>::Get()->NewIteration();

	--currIteration;
}

bool MagicInput::IsFinalIterationThisFrame() const
{
	return currIteration <= 1;
}

bool MagicInput::CreateInputSet(const std::string& name)
{
	return inputSets.try_emplace(name, std::make_shared<InputSet>()).second;
}

bool MagicInput::SwitchInputSet(const std::string& inputSetIdentifier)
{
	auto inputSetIter{ inputSets.find(inputSetIdentifier) };
	if (inputSetIter == inputSets.end())
		return false;

	currentInputSet = inputSetIter->second;
	return true;
}

bool MagicInput::RenameInputSet(const std::string& oldName, const std::string& newName)
{
	if (inputSets.find(newName) != inputSets.end() || inputSets.find(oldName) == inputSets.end())
		return false;

	inputSets.try_emplace(newName, inputSets.at(oldName));
	inputSets.erase(oldName);
	return true;
}

SPtr<const InputSet> MagicInput::GetCurrentInputSet() const
{
	return currentInputSet.lock();
}
SPtr<InputSet> MagicInput::GetCurrentInputSet()
{
	return currentInputSet.lock();
}

void MagicInput::Serialize(Serializer& writer) const
{
	writer.Serialize("inputSets", inputSets);
}

void MagicInput::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("inputSets", &inputSets, [](Deserializer& inReader, decltype(inputSets)* map) -> void {
		std::pair<std::string, SPtr<InputSet>> pair{ "", std::make_shared<InputSet>() };
		inReader.DeserializeVar("key", &pair.first);
		inReader.Deserialize("value", pair.second.get());
		map->emplace(std::move(pair));
	});
}

decltype(util::ToSortedVectorOfRefs(MagicInput::inputSets)) MagicInput::Editor_GetInputSets()
{
	return util::ToSortedVectorOfRefs(inputSets);
}

WPtr<InputSet> MagicInput::Editor_GetCurrentInputSet()
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
