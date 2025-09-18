/******************************************************************************/
/*!
\file   EntitySpawnEvents.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/22/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for a class that registers callback functions listening
  for entity creation events.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EntitySpawnEvents.h"

#include "NameComponent.h"
#include "EntityUID.h"
#include "EntityLayers.h"

EntitySpawnEvents::EntitySpawnEvents()
	: entityCreatedEventName{ "OnEntityCreated" }
{
	RegisterAttachEvent<NameComponent>();
	RegisterAttachEvent<EntityUIDComponent>();
	RegisterAttachEvent<EntityEventsComponent>();
	RegisterAttachEvent<EntityLayerComponent>();
}

EntitySpawnEvents::~EntitySpawnEvents()
{
	UnregisterAll();
}

void EntitySpawnEvents::RegisterEvent(EntitySpawnEvents::CallbackFuncSig func)
{
	registeredFunctions.insert(func);
	Messaging::Subscribe(entityCreatedEventName, func);
}

void EntitySpawnEvents::UnregisterEvent(EntitySpawnEvents::CallbackFuncSig func)
{
	registeredFunctions.erase(func);
	Messaging::Unsubscribe(entityCreatedEventName, func);
}

void EntitySpawnEvents::UnregisterAll()
{
	for (CallbackFuncSig callbackFunc : registeredFunctions)
		Messaging::Unsubscribe(entityCreatedEventName, callbackFunc);
	registeredFunctions.clear();
}
