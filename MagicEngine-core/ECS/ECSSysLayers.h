/******************************************************************************/
/*!
\file   ECSSysLayers.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This file exists to standardize layers when adding systems into ECS, and is not included with
  the ECS header to hopefully reduce recompilations required when more layers are added.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\brief
	Enums that identify a system layer in ecs.
*//******************************************************************/
enum class ECS_LAYER : int
{
	CUTOFF_START, // --- UNUSED

	//! The realtime input layer, for systems that run at input, regardless of timescale.
	REALTIME_INPUT_0,

	CUTOFF_REALTIME_INPUT, // --- UNUSED

	//! The input layer, for systems that run at player input.
	INPUT_0,

	CUTOFF_INPUT, // --- UNUSED

	//! For systems that want to run before all other usual systems.
	PRE_UPDATE_0,

	//! The tweening layer, for systems that run when tweens update.
	TWEENING,

	//! The audio layer, for systems that interact primarily with AudioManager.
	AUDIO,

	//! The pre-physics layer, for systems that run before physics.
	PRE_PHYSICS_0,

	CUTOFF_PRE_PHYSICS, // --- UNUSED

	SCRIPT_PREAWAKE,
	SCRIPT_AWAKE,
	SCRIPT_START,
	SCRIPT_UPDATE,

	CUTOFF_PRE_PHYSICS_SCRIPTS, // --- UNUSED

	//! The physics layer, for systems that during physics update step.
	PHYSICS,
	//! The collision layer, for systems that run during collision check and resolution.
	COLLISION,

	CUTOFF_PHYSICS, // --- UNUSED

	//! The post-physics layer, for systems that run after physics.
	POST_PHYSICS_0,
	POST_PHYSICS_1,
	POST_PHYSICS_2,
	POST_PHYSICS_3,

	CUTOFF_POST_PHYSICS, // --- UNUSED

	SCRIPT_LATE_UPDATE,

	CUTOFF_POST_PHYSICS_SCRIPTS, // --- UNUSED

	//! The render layer, for systems that run during the render step of the world.
	RENDER_0,
	RENDER_1,

	//! The render UI layer, for systems that run during the UI render step.
	RENDER_UI_0,

	CUTOFF_RENDER // --- UNUSED
};

namespace ecs {

	/*****************************************************************//*!
	\brief
		Runs systems in between the 2 specified layers.
	\param prevCutoff
		The previous cutoff layer. No systems are run in this layer.
	\param untilCutoff
		Execution will stop when reaching this cutoff layer. No systems are run in this layer.
	*//******************************************************************/
	void RunSystemsInLayers(ECS_LAYER prevCutoff, ECS_LAYER untilOffset);

}
