/******************************************************************************/
/*!
\file   Collision.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   06/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains definition of member functions for collision layers and broad phase
	collision layers.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Collision.h"

namespace physics {
	bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
	{
		switch (inObject1)
		{
		case +Layers::NON_MOVING:
			return +inObject2 == +Layers::MOVING; // Non moving only collides with moving
		case +Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	BPLayerInterfaceImpl::BPLayerInterfaceImpl()
	{
		mObjectToBroadPhase[+Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[+Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	JPH::uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		JPH_ASSERT(+inLayer < +Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}


	// TODO could not get this to compile. need to revisit later.
	/*const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer))
		{
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
			return "NON_MOVING";
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
			return "MOVING";
		default:
			JPH_ASSERT(false);
			return "INVALID";
		}
	}*/

	bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
		case +Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case +Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
}