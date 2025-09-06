/******************************************************************************/
/*!
\file   Collision.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   06/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains the definition of collision layers and the mapping between the 
	different layers.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

namespace physics {
	/*****************************************************************//*!
	\brief
		Layer that objects can be in, determines which other objects 
		it can collide with. Typically you at least want to have 1 
		layer for moving bodies and 1 layer for static bodies, but 
		you can have more layers if you want. E.g. you could have a 
		layer for high detail collision (which is not used by the physics 
		simulation but only if you do collision testing).
	*//******************************************************************/
	enum class Layers : JPH::ObjectLayer
	{
		NON_MOVING = 0,
		MOVING,
		NUM_LAYERS
	};

	/*****************************************************************//*!
	\brief
		Each broadphase layer results in a separate bounding volume 
		tree in the broad phase. You at least want to have a layer for 
		non-moving and moving objects to avoid having to update a tree 
		full of static objects every frame. You can have a 1-on-1 mapping 
		between object layers and broadphase layers (like in this case) 
		but if you have many object layers you'll be creating many broad 
		phase trees, which is not efficient. If you want to fine tune
		your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look 
		at the stats reported on the TTY.
	*//******************************************************************/
	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::uint NUM_LAYERS(2);
	};

	/*****************************************************************//*!
	\brief
		Class that determines if two object layers can collide
	*//******************************************************************/
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
	};

	/*****************************************************************//*!
	\brief
		BroadPhaseLayerInterface implementation
		This defines a mapping between object and broadphase layers.
	*//******************************************************************/
	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BPLayerInterfaceImpl();

		virtual JPH::uint GetNumBroadPhaseLayers() const override;

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[static_cast<JPH::uint16>(Layers::NUM_LAYERS)];
	};

	/*****************************************************************//*!
	\brief
		Class that determines if an object layer can collide with a broadphase layer
	*//******************************************************************/
	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
	};

}