/* DEPRECATED - Planned to refactor into a new redesigned input system. */


/******************************************************************************/
/*!
\file   GamepadInputAdapter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a gamepad input adapter system for this game.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\class GamepadAimAdapterComponent
\brief
	Marks an entity as the reference point for gamepad aiming.
*//******************************************************************/
class GamepadAimAdapterComponent : public IRegisteredComponent<GamepadAimAdapterComponent>
{
public:
	property_vtable()
};
property_begin(GamepadAimAdapterComponent)
{
}
property_vend_h(GamepadAimAdapterComponent)

/*****************************************************************//*!
\class GamepadAimAdapterSystem
\brief
	Processes GamepadAimAdapterComponent according to gamepad input.
*//******************************************************************/
class GamepadAimAdapterSystem : public ecs::System<GamepadAimAdapterSystem, GamepadAimAdapterComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	GamepadAimAdapterSystem();

private:
	/*****************************************************************//*!
	\brief
		Updates component based on gamepad input.
	*//******************************************************************/
	void UpdateComp(GamepadAimAdapterComponent& comp);

};

/*****************************************************************//*!
\class GamepadMouseControlComponent
\brief
	Marks a scene as controllable by controller.
*//******************************************************************/
class GamepadMouseControlComponent : public IRegisteredComponent<GamepadMouseControlComponent>
{
public:
	property_vtable()
};
property_begin(GamepadMouseControlComponent)
{
}
property_vend_h(GamepadMouseControlComponent)

/*****************************************************************//*!
\class GamepadMouseControlSystem
\brief
	Processes GamepadMouseControlComponent according to gamepad input.
*//******************************************************************/
class GamepadMouseControlSystem : public ecs::System<GamepadMouseControlSystem, GamepadMouseControlComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	GamepadMouseControlSystem();

private:
	/*****************************************************************//*!
	\brief
		Updates component based on gamepad input.
	*//******************************************************************/
	void UpdateComp(GamepadMouseControlComponent& comp);
};
