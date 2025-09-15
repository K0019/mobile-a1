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

namespace internal {

	class InputTrigger
	{
	public:

	private:

	};

	class InputDeviceBase
	{
	public:

	private:

	};

}

#pragma region Keyboard/Mouse

/*****************************************************************//*!
\enum KEY
\brief
	Identifies a key.
*//******************************************************************/
enum class KEY : int
{
	A = GLFW_KEY_A,
	B = GLFW_KEY_B,
	C = GLFW_KEY_C,
	D = GLFW_KEY_D,
	E = GLFW_KEY_E,
	F = GLFW_KEY_F,
	G = GLFW_KEY_G,
	H = GLFW_KEY_H,
	I = GLFW_KEY_I,
	J = GLFW_KEY_J,
	K = GLFW_KEY_K,
	L = GLFW_KEY_L,
	M = GLFW_KEY_M,
	N = GLFW_KEY_N,
	O = GLFW_KEY_O,
	P = GLFW_KEY_P,
	Q = GLFW_KEY_Q,
	R = GLFW_KEY_R,
	S = GLFW_KEY_S,
	T = GLFW_KEY_T,
	U = GLFW_KEY_U,
	V = GLFW_KEY_V,
	W = GLFW_KEY_W,
	X = GLFW_KEY_X,
	Y = GLFW_KEY_Y,
	Z = GLFW_KEY_Z,
	NUM_0 = GLFW_KEY_0,
	NUM_1 = GLFW_KEY_1,
	NUM_2 = GLFW_KEY_2,
	NUM_3 = GLFW_KEY_3,
	NUM_4 = GLFW_KEY_4,
	NUM_5 = GLFW_KEY_5,
	NUM_6 = GLFW_KEY_6,
	NUM_7 = GLFW_KEY_7,
	NUM_8 = GLFW_KEY_8,
	NUM_9 = GLFW_KEY_9,
	SPACE = GLFW_KEY_SPACE,
	ENTER = GLFW_KEY_ENTER,
	ESC = GLFW_KEY_ESCAPE,
	GRAVE = GLFW_KEY_GRAVE_ACCENT,

	UP = GLFW_KEY_UP,
	RIGHT = GLFW_KEY_RIGHT,
	DOWN = GLFW_KEY_DOWN,
	LEFT = GLFW_KEY_LEFT,

	LCTRL = GLFW_KEY_LEFT_CONTROL,
	LSHIFT = GLFW_KEY_LEFT_SHIFT,
	LALT = GLFW_KEY_LEFT_ALT,

	DEL = GLFW_KEY_DELETE,

	F1 = GLFW_KEY_F1,
	F2 = GLFW_KEY_F2,
	F3 = GLFW_KEY_F3,
	F4 = GLFW_KEY_F4,
	F5 = GLFW_KEY_F5,
	F6 = GLFW_KEY_F6,
	F7 = GLFW_KEY_F7,
	F8 = GLFW_KEY_F8,
	F9 = GLFW_KEY_F9,
	F10 = GLFW_KEY_F10,
	F11 = GLFW_KEY_F11,
	F12 = GLFW_KEY_F12,

	M_LEFT = GLFW_MOUSE_BUTTON_LEFT,
	M_RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
	M_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
	M1 = GLFW_MOUSE_BUTTON_LEFT,
	M2 = GLFW_MOUSE_BUTTON_RIGHT,
	M3 = GLFW_MOUSE_BUTTON_MIDDLE,
};
GENERATE_ENUM_CLASS_ARITHMETIC_OPERATORS(KEY)

class KeyboardMouseInput : public internal::InputDeviceBase
{
public:

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

class Input
{
public:
	// Disable instancing of this class
	Input() = delete;

	/*****************************************************************//*!
	 \brief
		  Reset the previous state to the current state
    *//******************************************************************/
	static void NewFrame();

	/*****************************************************************//*!
	\brief
		To avoid multiple Press/Release events in catch up frames, this function
		updates the state of key press/release only on iterations where it makes sense.
		> Press is only true on the first iteration.
		> Release is only true on the last iteration.
	*//******************************************************************/
	static void NewIteration();

	/*****************************************************************//*!
	   \brief
		   Sets a key to pressed state.
	   \param key
		   The GLFW key code.
   *//******************************************************************/
	static void OnKeyDown(short key);

	/*****************************************************************//*!
		\brief
		   Sets a key to unpressed state.
		\param key
			The GLFW key code.
	*//******************************************************************/
	static void OnKeyUp(short key);


	/*****************************************************************//*!
		\brief
			Sets the mouse position.
		\param mouseX
			Mouuse X pos
		\param mouseY
			Mouse Y pos
	*//******************************************************************/
	static void OnMouseMove(double mouseX, double mouseY);
	

	/*****************************************************************//*!
		\brief
			Gets whether a key is currently pressed.
		\param key
			Key identifier.
		\return
			True if the key is currently pressed. False otherwise.
	*//******************************************************************/
	static bool GetKeyCurr(KEY key);

	/*****************************************************************//*!
		\brief
			Gets whether a key was pressed this frame.
		\param key
			Key identifier
		\return
			True if the key was pressed this frame. False otherwise.
	*//******************************************************************/
	static bool GetKeyPressed(KEY key);

	/*****************************************************************//*!
		\brief
			Gets whether a key was released this frame.
		\param key
			Key identifier
		\return
			True if the key was released this frame. False otherwise.
	*//******************************************************************/
	static bool GetKeyReleased(KEY key);

	/*****************************************************************//*!
	\brief
		Gets the mouse's current window position.
	\return
		The mouse's current window position.
	*//******************************************************************/
	static const Vec2& GetMousePosRaw();

	/*****************************************************************//*!
	\brief
		Gets the mouse's current position in the world.
		TODO 3D: Determine whether mouse world position should be Vec2, Vec3 or not exist at all.
	\return
		The mouse's current window in the world.
	*//******************************************************************/
	static Vec3 GetMousePosWorld();

	static void OnScroll(float offset);

	static float GetScroll();

private:
	// Mouse state is combined into key states (as their indexes don't clash)
	//! The current state of keys.
	static std::bitset<GLFW_KEY_LAST + 1> keystate;
	//! Whether keys were pressed since the last update.
	static std::bitset<GLFW_KEY_LAST + 1> pressedKeystate;
	//! Whether keys were released since the last update.
	static std::bitset<GLFW_KEY_LAST + 1> releasedKeystate;

	//! The current mouse window position.
	static Vec2 mousePos;
	static float scrollOffset;

	//! Tracks which iteration we're at.
	static int currIteration;
};

/*****************************************************************//*!
\class GamepadInput
\brief
	Processes gamepad input from GLFW and updates the input system accordingly.
*//******************************************************************/
class GamepadInput
{
public:
	/*****************************************************************//*!
	\brief
		Polls gamepad input.
	*//******************************************************************/
	static void PollInput();

private:
	//! The previous state of the gamepad.
	static GLFWgamepadstate prevState;
	//! Whether the gamepad is active. (incomplete)
	static bool gamepadActive;

};
