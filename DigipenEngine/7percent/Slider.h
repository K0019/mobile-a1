#include "GameSettings.h"
#include "ResourceManager.h"
#include "RenderSystem.h"
#include "ComponentLookupWorkaround.h"

/******************************************************************************/
/*!
\file   Slider.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   13/03/2025

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
	Header for slider component to adjust volume

All content  © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#define M_SOUND_TYPE \
X(SFX, "SFX") \
X(BGM, "BGM") 


#define X(enumName, strName) enumName,
enum class VOLUME : int
{
	M_SOUND_TYPE
};
#undef X



class SliderComponent : public IRegisteredComponent<SliderComponent>, ecs::IComponentCallbacks
#ifdef IMGUI_ENABLED
	, IEditorComponent <SliderComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Constructor
	*//******************************************************************/
	SliderComponent();

	/*****************************************************************//*!
	\brief
		Function runs when component is attached to an entity
	*//******************************************************************/
	void OnAttached() override;

	/*****************************************************************//*!
	\brief
		Function runs when component is dettached to an entity
	*//******************************************************************/
	void OnDetached() override;

	/*****************************************************************//*!
	\brief
		Returns result of slider being pressed in current frame
	*//******************************************************************/
	bool GetIsPressed();

	size_t GetSpriteIDPressed();
	size_t GetSpriteIDUnPressed();

	/*****************************************************************//*!
	\brief
		Returns current value of slider
	*//******************************************************************/
	float GetSliderValue();
	bool GetInitialSpriteSet();


	void SetSpriteIDPressed(size_t val);
	void SetSpriteIDUnPressed(size_t val);

	/*****************************************************************//*!
	\brief
		Sets the result of the slider being clicked on int eh current frame
	*//******************************************************************/
	void SetIsPressed(bool val);

	/*****************************************************************//*!
	\brief
		Sets the slider value
	*//******************************************************************/
	void SetSliderValue(float val);
	void SetInitialSpriteSet(bool val);
	

	int sound;

private:
#ifdef IMGUI_ENABLED
	static void EditorDraw(SliderComponent& comp);
#endif

	bool isPressed;
	size_t	spriteID_Pressed;
	size_t	spriteID_Unpressed;
	float sliderValue; // 0.f to 1.f
	bool setInitial;
	

	property_vtable()
};
property_begin(SliderComponent)
{
	property_var(isPressed),
		property_var(spriteID_Pressed),
		property_var(spriteID_Unpressed),
		property_var(sliderValue),
		property_var(sound)
}
property_vend_h(SliderComponent)


/*****************************************************************//*!
	\brief
		slider system
*//******************************************************************/
class SliderSystem : public ecs::System<SliderSystem, SliderComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	SliderSystem();

private:
	void UpdateSliderComp(SliderComponent& comp);

	/*****************************************************************//*!
	\brief
		Function gets called when slider button is being held 
	*//******************************************************************/
	void onSliderHold(SliderComponent& comp);
};