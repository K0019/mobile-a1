#pragma once
/******************************************************************************/
/*!
\file   Input.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/26/2024

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
	This is the header file for the game's input movement system.
	Any buttion related input has been enumed

	TODO: Refactor this system to include an intermediate action mapping layer.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include <windows.h>
#include <GLFW/glfw3.h>
#include <bitset>

#include "MagicMath.h"
#include "MacroTemplates.h"

// Forward declaration
class InputConfig;


namespace internal {

	static constexpr float DEADZONE = 0.15f;

	class InputDeviceBase
	{
	public:

	private:

	};

}

#define ENUM_INPUT_DEVICE_TYPE \
X(KEYBOARD_MOUSE, "Keyboard/Mouse") \
X(GAMEPAD, "Gamepad")

#define X(name, str) name,
	enum class INPUT_DEVICE_TYPE : char
	{
		ENUM_INPUT_DEVICE_TYPE
		NUM_DEVICES,
		INVALID_DEVICE = -1
	};
#undef X

#pragma region Bindings

enum class INPUT_READ_TYPE : char
{
	// Pressed/Released THIS FRAME
	PRESSED,
	RELEASED,
	// CURRENTLY up/down
	CURRENT
};

#pragma region Hardware Link

using InputHardwareValue = std::variant<bool, float>;
class InputHardwareValueLink
{
public:
	InputHardwareValueLink();
	InputHardwareValueLink(INPUT_DEVICE_TYPE deviceType, int keyIdentifier, INPUT_READ_TYPE readType);

	// Gets the value of the key linked at creation time
	InputHardwareValue ReadValue() const;

	// Gets the hardware device that this reads from
	INPUT_DEVICE_TYPE GetDeviceType() const;
	void SetDeviceType(INPUT_DEVICE_TYPE newDeviceType);

	int GetKeyIdentifier() const;
	void SetKeyIdentifier(int newKeyIdentifier);

protected:
	//! Which input device to read from
	INPUT_DEVICE_TYPE deviceType;
	//! Whether to check if the key was pressed/released this frame only, or just get the current value
	INPUT_READ_TYPE readType;
	//! The identifier for the input key on the device
	int keyIdentifier;

private:
	// To avoid virtualization based on device type, we're storing virtualized behaviors in these function pointer arrays.

	//! Set of functions that query a particular input device for a key value
	using FuncType_GetValue = std::variant<bool, float>(*)(int keyIdentifier, INPUT_READ_TYPE readType);
	static const std::array<FuncType_GetValue, +INPUT_DEVICE_TYPE::NUM_DEVICES> GetValueFromDevice;
};

#pragma endregion // Hardware Link

#pragma region Input Binding

#define ENUM_INPUT_COMPOSITE_TYPE \
X(BUTTON, "Button") /* Bool for either key is up or down */ \
X(AXIS_1D, "1D Axis") /* Float value from -1 to 1, supports 2 hardware values for positive/negative */ \
X(AXIS_2D, "2D Axis") /* Vec2 value from -1 to 1 on each axis */

#define X(name, str) name,
enum class INPUT_COMPOSITE_TYPE
{
	ENUM_INPUT_COMPOSITE_TYPE
	NUM_TYPES
};
#undef X

template <INPUT_COMPOSITE_TYPE CompositeType>
class InputBinding
{
public:
	// Get the input value of this binding. Return type depends on the composite type.
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::BUTTON, bool> GetValue() const;
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_1D, float> GetValue() const;
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_2D, Vec2> GetValue() const;

	template <INPUT_COMPOSITE_TYPE NewCompositeType>
	InputBinding<NewCompositeType> ConvertToCompositeType() const;

private:
	// Gets and converts an InputHardwareValueLink's value to the desired value type.
	template <typename ValueType>
	static ValueType Get(const InputHardwareValueLink& hardwareValueLink);

private:
	//! The values of this binding will be read from these hardware values.
	//! Store 1, 2 or 4 InputHardwareValueLink depending on the composite type.
	[[msvc::no_unique_address]] util::OptionalVar<CompositeType == INPUT_COMPOSITE_TYPE::BUTTON, InputHardwareValueLink> hardwareValues_Button;
	[[msvc::no_unique_address]] util::OptionalVar<CompositeType == INPUT_COMPOSITE_TYPE::AXIS_1D, std::array<InputHardwareValueLink, 2>> hardwareValues_1D;
	[[msvc::no_unique_address]] util::OptionalVar<CompositeType == INPUT_COMPOSITE_TYPE::AXIS_2D, std::array<InputHardwareValueLink, 4>> hardwareValues_2D;

public:
	template <typename FuncType>
	void Editor_ForEachHardwareValueLink(FuncType func);
};

#pragma endregion // Input Binding

#pragma region Input Action

class InputActionBase
{
public:
	InputActionBase(INPUT_COMPOSITE_TYPE compositeType);

	INPUT_COMPOSITE_TYPE GetCompositeType() const;

	virtual void AddBinding() = 0;

private:
	INPUT_COMPOSITE_TYPE compositeType;

};

template <INPUT_COMPOSITE_TYPE CompositeType>
class InputAction : public InputActionBase
{
public:
	InputAction();

	// Get the input value of this action. Return type depends on the composite type.
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::BUTTON, bool> GetValue() const;
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_1D, float> GetValue() const;
	template <INPUT_COMPOSITE_TYPE T = CompositeType>
	std::enable_if_t<T == INPUT_COMPOSITE_TYPE::AXIS_2D, Vec2> GetValue() const;

	virtual void AddBinding() final;
	const InputBinding<CompositeType>* GetBinding(size_t index) const;
	InputBinding<CompositeType>* GetBinding(size_t index);

	template <INPUT_COMPOSITE_TYPE NewCompositeType>
	InputAction<NewCompositeType> ConvertToCompositeType() const;
	template <INPUT_COMPOSITE_TYPE OriginalCompositeType>
	void INTERNAL_ConvertAndSetBindings(const std::vector<InputBinding<OriginalCompositeType>>& originalBindings);

private:
	//! The input bindings that "activate" this action.
	std::vector<InputBinding<CompositeType>> bindings;

public:
	std::vector<InputBinding<CompositeType>>& Editor_GetBindings();

};

#pragma endregion // Input Action

#pragma region Input Set

class InputSet
{
public:
	// Creates a button action
	bool CreateNewAction(const std::string& name);

	SPtr<const InputActionBase> GetAction(const std::string& name) const;
	SPtr<InputActionBase> GetAction(const std::string& name);
	void SetAction(const std::string& name, SPtr<InputActionBase>&& action);

private:
	std::unordered_map<std::string, SPtr<InputActionBase>> actions;

public:
	decltype(util::ToSortedVectorOfRefs(actions)) Editor_GetActions();

};

#pragma endregion Input Set

#pragma endregion // Bindings

#pragma region Keyboard/Mouse

/*****************************************************************//*!
\enum KEY
\brief
	Identifies a key.
*//******************************************************************/
#define ENUM_KEY \
X(A, GLFW_KEY_A, "A") \
X(B, GLFW_KEY_B, "B") \
X(C, GLFW_KEY_C, "C") \
X(D, GLFW_KEY_D, "D") \
X(E, GLFW_KEY_E, "E") \
X(F, GLFW_KEY_F, "F") \
X(G, GLFW_KEY_G, "G") \
X(H, GLFW_KEY_H, "H") \
X(I, GLFW_KEY_I, "I") \
X(J, GLFW_KEY_J, "J") \
X(K, GLFW_KEY_K, "K") \
X(L, GLFW_KEY_L, "L") \
X(M, GLFW_KEY_M, "M") \
X(N, GLFW_KEY_N, "N") \
X(O, GLFW_KEY_O, "O") \
X(P, GLFW_KEY_P, "P") \
X(Q, GLFW_KEY_Q, "Q") \
X(R, GLFW_KEY_R, "R") \
X(S, GLFW_KEY_S, "S") \
X(T, GLFW_KEY_T, "T") \
X(U, GLFW_KEY_U, "U") \
X(V, GLFW_KEY_V, "V") \
X(W, GLFW_KEY_W, "W") \
X(X, GLFW_KEY_X, "X") \
X(Y, GLFW_KEY_Y, "Y") \
X(Z, GLFW_KEY_Z, "Z") \
X(NUM_0, GLFW_KEY_0, "Number 0")	\
X(NUM_1, GLFW_KEY_1, "Number 1")	\
X(NUM_2, GLFW_KEY_2, "Number 2")	\
X(NUM_3, GLFW_KEY_3, "Number 3")	\
X(NUM_4, GLFW_KEY_4, "Number 4")	\
X(NUM_5, GLFW_KEY_5, "Number 5")	\
X(NUM_6, GLFW_KEY_6, "Number 6")	\
X(NUM_7, GLFW_KEY_7, "Number 7")	\
X(NUM_8, GLFW_KEY_8, "Number 8")	\
X(NUM_9, GLFW_KEY_9, "Number 9")	\
X(SPACE, GLFW_KEY_SPACE, "Space") \
X(ENTER, GLFW_KEY_ENTER, "Enter") \
X(ESC, GLFW_KEY_ESCAPE, "Escape") \
X(GRAVE, GLFW_KEY_GRAVE_ACCENT, "Grave") \
X(UP, GLFW_KEY_UP, "Up Arrow") \
X(RIGHT, GLFW_KEY_RIGHT, "Right Arrow") \
X(DOWN, GLFW_KEY_DOWN, "Down Arrow") \
X(LEFT, GLFW_KEY_LEFT, "Left Arrow") \
X(LCTRL, GLFW_KEY_LEFT_CONTROL, "Left Control") \
X(LSHIFT, GLFW_KEY_LEFT_SHIFT, "Left Shift") \
X(LALT, GLFW_KEY_LEFT_ALT, "Left Alt") \
X(DEL, GLFW_KEY_DELETE, "Delete") \
X(F1, GLFW_KEY_F1, "F1") \
X(F2, GLFW_KEY_F2, "F2") \
X(F3, GLFW_KEY_F3, "F3") \
X(F4, GLFW_KEY_F4, "F4") \
X(F5, GLFW_KEY_F5, "F5") \
X(F6, GLFW_KEY_F6, "F6") \
X(F7, GLFW_KEY_F7, "F7") \
X(F8, GLFW_KEY_F8, "F8") \
X(F9, GLFW_KEY_F9, "F9") \
X(F10, GLFW_KEY_F10, "F10") \
X(F11, GLFW_KEY_F11, "F11") \
X(F12, GLFW_KEY_F12, "F12") \
X(M1, GLFW_MOUSE_BUTTON_LEFT, "Left Mouse Button") \
X(M2, GLFW_MOUSE_BUTTON_RIGHT, "Right Mouse Button") \
X(M3, GLFW_MOUSE_BUTTON_MIDDLE, "Middle Mouse Button")

#define X(name, glfw, str) name = glfw,
enum class KEY : int
{
	ENUM_KEY
	M_LEFT = M1,
	M_RIGHT = M2,
	M_MIDDLE = M3,
};
#undef X
GENERATE_ENUM_CLASS_ARITHMETIC_OPERATORS(KEY)

class KeyboardMouseInput : public internal::InputDeviceBase
{
public:
	bool GetIsPressed(KEY key) const;
	bool GetIsReleased(KEY key) const;
	bool GetIsDown(KEY key) const;
	bool GetValue(INPUT_READ_TYPE readType, KEY key) const;
	float GetScroll() const;

public:
	// Frame management
	void NewFrame();
	void NewIteration();

public:
	static void GLFW_Callback_OnKeyboardClick(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void GLFW_Callback_OnMouseClick(GLFWwindow* window, int button, int action, int mode);
	static void GLFW_Callback_OnMouseMove(GLFWwindow* window, double xpos, double ypos);
	static void GLFW_Callback_OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);

private:
	void Callback_OnKeyDown(short key);
	void Callback_OnKeyUp(short key);
	void Callback_OnMouseMove(double x, double y);
	void Callback_OnMouseScroll(float offset);

private:
	// Mouse state is combined into key states (as their indexes don't clash)
	//! The current state of keys.
	std::bitset<GLFW_KEY_LAST + 1> keystate;
	//! Whether keys were pressed since the last update.
	std::bitset<GLFW_KEY_LAST + 1> pressedKeystate;
	//! Whether keys were released since the last update.
	std::bitset<GLFW_KEY_LAST + 1> releasedKeystate;

	//! The current mouse window position.
	Vec2 mousePos;
	float scrollOffset;
};

#pragma endregion // Keyboard/Mouse

#pragma region New Interface

class Input
{
public:
	bool CreateInputSet(const std::string& name);
	bool SwitchInputSet(const std::string& inputSetIdentifier);

	SPtr<const InputSet> GetCurrentInputSet() const;
	SPtr<InputSet> GetCurrentInputSet();

	template <INPUT_COMPOSITE_TYPE CompositeType>
	SPtr<const InputAction<CompositeType>> GetAction(const std::string& actionName) const;

public: // Frame management
	/*****************************************************************//*!
	 \brief
		  Reset the previous state to the current state
	*//******************************************************************/
	void NewFrame();

	/*****************************************************************//*!
	\brief
		To avoid multiple Press/Release events in catch up frames, this function
		updates the state of key press/release only on iterations where it makes sense.
		> Press is only true on the first iteration.
		> Release is only true on the last iteration.
	*//******************************************************************/
	void NewIteration();

	/*****************************************************************//*!
	 \brief
		Checks whether the current iteration of the game systems is the
		final iteration for this current frame.
	*//******************************************************************/
	bool IsFinalIterationThisFrame() const;

private:
	std::unordered_map<std::string, SPtr<InputSet>> inputSets;
	WPtr<InputSet> currentInputSet;

	//! Tracks which iteration we're at.
	int currIteration;

public:
	// For InputConfig to get and modify input sets
	decltype(util::ToSortedVectorOfRefs(inputSets)) Editor_GetInputSets();
	WPtr<InputSet> Editor_GetCurrentInputSet();
};

#pragma endregion // New Interface

#pragma region Old Interface

/*****************************************************************//*!
\class GamepadInput
\brief
	Processes gamepad input from GLFW and updates the input system accordingly.
*//******************************************************************/
//class GamepadInput
//{
//public:
//	/*****************************************************************//*!
//	\brief
//		Polls gamepad input.
//	*//******************************************************************/
//	static void PollInput();
//
//private:
//	//! The previous state of the gamepad.
//	static GLFWgamepadstate prevState;
//	//! Whether the gamepad is active. (incomplete)
//	static bool gamepadActive;
//
//};

#pragma endregion // Old Interface

#include "Input.ipp"
