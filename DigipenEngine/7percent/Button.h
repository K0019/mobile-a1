/******************************************************************************/
/*!
\file   Button.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Chua Wen Shing Bryan (25%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\author Ryan Cheong (35%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (40%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Header file for declarations of fuctions for button component. 
	Button component allows any enitities that it is attached to become 
	a button. 
	It has a pressed and unpressed state and the png for both states can
	be changed

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "RenderSystem.h"
#include "SceneManagement.h"
#include "AudioManager.h"

class ButtonComponent : public IRegisteredComponent<ButtonComponent>,
	ecs::IComponentCallbacks
#ifdef IMGUI_ENABLED
	, IEditorComponent<ButtonComponent>
#endif
{
public:
	// Number of functions
	int numFunctions;
	// Store functions to call and parameters to input into functions
	std::vector<std::pair<std::string, std::string>> vecFunctions;

	/*****************************************************************//*!
	 \brief
		  Default constructor.
	*//******************************************************************/
	ButtonComponent();

	/*****************************************************************//*!
	\brief
		Gets whether this button is a targeted button (was clicked on initially).
	\return
		Whether this button is a targeted button.
	*//******************************************************************/
	bool GetIsPressTarget() const;

	/*****************************************************************//*!
	\brief
		Sets whether this button is a targeted button (was clicked on initially).
	\param newIsPressTarget
		Whether this button is a targeted button.
	*//******************************************************************/
	void SetIsPressTarget(bool newIsPressTarget);

	/*****************************************************************//*!
	\brief
		Sets the state of this button, but does no action.
	\param isPressed
		Whether this button is pressed.
	*//******************************************************************/
	void SetIsPressed(bool isPressed);

	/*****************************************************************//*!
	 \brief
			Gets SpriteID_Pressed variable
	 \return
		   size_t
	*//******************************************************************/
	size_t	GetSpriteIDPressed();

	/*****************************************************************//*!
	 \brief
			Gets SpriteID_UnPressed variable
	 \return
		   size_t
	*//******************************************************************/
	size_t  GetSpriteIDUnPressed();

	/*****************************************************************//*!
	 \brief
			Sets spriteIDPressed variable
	 \return
		   void
	*//******************************************************************/
	void SetSpriteIDPressed(size_t val);

	/*****************************************************************//*!
	 \brief
			Sets spriteIDUnPressde variable
	 \return
		   void
	*//******************************************************************/
	void SetSpriteIDUnPressed(size_t val);

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	bool GetIsPressable() const;
	void Enable();
	void Disable();

	void OnAttached() override;
	void OnDetached() override;
private:

	/*****************************************************************//*!
	 \brief
			draws the 2 boxes in the engine for user to drag and drop pngs
			into one of the boxes to set pressed/unpressed png
	 \return
		   void
	*//******************************************************************/
#ifdef IMGUI_ENABLED
	static void EditorDraw(ButtonComponent& comp);
#endif

	//! The previous state of the button, to facilitate audio playback
	bool wasPressed;

	//! Whether the button can be pressed, allows disabling of buttons
	bool isPressable;

	//! Whether this button was pressed on the "pressed" event (mouse down), so we can track which button should
	//! change its texture if the mouse cursor moves on/off the button.
	bool isPressTarget;

	size_t	spriteID_Pressed;
	size_t	spriteID_Unpressed;
	AudioReference clickSound;

	property_vtable()
};
property_begin(ButtonComponent)
{
	property_var(spriteID_Pressed),
	property_var(spriteID_Unpressed),
	property_var(clickSound),
	property_var(numFunctions)
}
property_vend_h(ButtonComponent)

/*****************************************************************//*!
\class ButtonSystem
\brief

*//******************************************************************/
class ButtonSystem : public ecs::System<ButtonSystem, ButtonComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	ButtonSystem();

	/*****************************************************************//*!
	\brief
		Checks the state of buttons.
	\return
		True.
	*//******************************************************************/
	bool PreRun() override;

private:
	/*****************************************************************//*!
	\brief
		Updates a button component.
	\param comp
		The ButtonComponent to update.
	*//******************************************************************/
	void UpdateButtonComp(ButtonComponent& comp);

	/*****************************************************************//*!
	\brief
		Called when the button is "clicked".
	\param comp
		The ButtonComponent to update.
	*//******************************************************************/
	void OnButtonClicked(ButtonComponent& comp);

private:
	bool isPressed;
	bool isCurrPressed;
	bool isReleased;

};