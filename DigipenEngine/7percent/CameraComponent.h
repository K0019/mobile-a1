#pragma once
/******************************************************************************/
/*!
\file   CameraComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (95%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (5%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Camera component that holds priority and active state.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

class CameraComponent : public IRegisteredComponent<CameraComponent>, public ecs::IComponentCallbacks
#ifdef IMGUI_ENABLED
	, IEditorComponent<CameraComponent>
#endif
{
	friend class CameraSystem;
public:
	explicit CameraComponent(bool active = false);

	bool isActive() const;
	void SetActive();

	/*****************************************************************//*!
	\brief
		Set active whenever a new camera component is attached.
	*//******************************************************************/
	void OnAttached() override;

private:
	bool active{ false };
	int priority{ 0 };
	float zoom{ 0.0f };
	static int globalPriority;
#ifdef IMGUI_ENABLED
	static void EditorDraw(CameraComponent& comp);
#endif
	property_vtable()
};
property_begin(CameraComponent)
{
	property_var(active),
	property_var(priority),
	property_var(zoom)
}
property_vend_h(CameraComponent)

/*****************************************************************//*!
\class AnchorToCameraComponent
\brief
	Anchors the attached entity to the camera's location.
*//******************************************************************/
class AnchorToCameraComponent : public IRegisteredComponent<AnchorToCameraComponent>
{
	property_vtable()
};
property_begin(AnchorToCameraComponent)
{
}
property_vend_h(AnchorToCameraComponent)

/*****************************************************************//*!
\class ShakeComponent
\brief
	Shakes the attached entity's position.
*//******************************************************************/
class ShakeComponent : public IRegisteredComponent<ShakeComponent>, public IEditorComponent<ShakeComponent>
{
public:
	/*****************************************************************//*!
	\struct Offsets
	\brief
		Contains the position/rotation offsets that this shake component is inducing.
	*//******************************************************************/
	struct Offsets
	{
		Vector2 pos;
		float rot;
	};

	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	ShakeComponent();

	/*****************************************************************//*!
	\brief
		Induce stress (trauma) that causes shaking.
	\param strength
		0 - 1 value of stress level.
	\param cap
		The maximum resulting trauma that this stress can induce.
	*//******************************************************************/
	void InduceStress(float strength, float cap = 1.0f);

	/*****************************************************************//*!
	\brief
		Updates this component's parameters based on time passed.
	\param dt
		Delta time.
	*//******************************************************************/
	void UpdateTime(float dt);

	/*****************************************************************//*!
	\brief
		Calculate offsets as a result of shaking.
	\return
		The offsets.
	*//******************************************************************/
	const Offsets& CalcOffsets();

	/*****************************************************************//*!
	\brief
		Gets the offsets as previously calculated.
	\return
		The offsets.
	*//******************************************************************/
	const Offsets& GetOffsets() const;

	/*****************************************************************//*!
	\brief
		Gets the current trauma value.
	\return
		The current trauma.
	*//******************************************************************/
	float GetTrauma() const;

private:
	static void EditorDraw(ShakeComponent& comp);

private:
	//! The strength of the shaking. 0 - 1.
	float trauma;
	//! How fast the shake strength falls off.
	float traumaExponent;
	//! The recovery speed of trauma.
	float recoverySpeed;
	//! How fast is the shaking
	float frequency;
	//! Accumulates time to determine the perlin noise location.
	float time;

	//! The maximum positional offset of the shake
	Vector2 maxPosOffset;
	//! The maximum rotational offset of the shake
	float maxRotOffset;

	//! The offsets applied to the entity.
	Offsets appliedOffsets;

public:
	property_vtable()
};
property_begin(ShakeComponent)
{
	property_var(trauma),
	property_var(traumaExponent),
	property_var(recoverySpeed),
	property_var(frequency),
	property_var(maxPosOffset),
	property_var(maxRotOffset)
}
property_vend_h(ShakeComponent)
