/******************************************************************************/
/*!
\file   EntityLayers.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for entity layers.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EntityLayers.h"

// Use X Macro to obtain enum name
#define X(name, str) str,
static const char* const entityLayerNames[]{
	D_ENTITY_LAYER
};
#undef X

EntityLayerComponent::EntityLayerComponent()
	: mask{ { ENTITY_LAYER::DEFAULT } }
{
}

void EntityLayerComponent::SetLayer(ENTITY_LAYER newLayer)
{
	ST<EntitiesByLayer>::Get()->MoveEntity(GetLayer(), newLayer, ecs::GetEntity(this));
	mask.SetMask(ENTITY_LAYER::ALL, false);
	mask.SetMask(newLayer, true);
}

ENTITY_LAYER EntityLayerComponent::GetLayer() const
{
	return mask.GetFirst1BitFromRight();
}

const char* EntityLayerComponent::GetLayerName(ENTITY_LAYER layer)
{
	return entityLayerNames[+layer];
}

void EntityLayerComponent::SerializeLayersMatrix(Serializer& writer, const std::string& key)
{
	EntityLayersMask::SerializeMatrix(writer, key, entityLayerNames);
}

void EntityLayerComponent::DeserializeLayersMatrix(Deserializer& reader, const std::string& key)
{
	EntityLayersMask::DeserializeMatrix(reader, key, entityLayerNames);
}

void EntityLayerComponent::OnAttached()
{
	ST<EntitiesByLayer>::Get()->AddEntity(GetLayer(), ecs::GetEntity(this));
}

void EntityLayerComponent::OnDetached()
{
	ST<EntitiesByLayer>::Get()->RemoveEntity(GetLayer(), ecs::GetEntity(this));
}

void EntityLayerComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("layer", +GetLayer());
}

void EntityLayerComponent::Deserialize(Deserializer& reader)
{
	std::underlying_type_t<ENTITY_LAYER> layerVal{};
	reader.DeserializeVar("layer", &layerVal);

	SetLayer(static_cast<ENTITY_LAYER>(layerVal));
}

void EntitiesByLayer::AddEntity(ENTITY_LAYER layer, ecs::EntityHandle entity)
{
	layers[+layer].insert(entity);
}

void EntitiesByLayer::MoveEntity(ENTITY_LAYER fromLayer, ENTITY_LAYER toLayer, ecs::EntityHandle entity)
{
	if (fromLayer == toLayer)
		return;
	RemoveEntity(fromLayer, entity);
	AddEntity(toLayer, entity);
}

void EntitiesByLayer::RemoveEntity(ENTITY_LAYER layer, ecs::EntityHandle entity)
{
	layers[+layer].erase(entity);
}

size_t EntitiesByLayer::GetNumEntitiesInLayers(EntityLayersMask layersMask)
{
	size_t count{};
	layersMask.ExecutePerActiveBit([&count, &layers = layers](ENTITY_LAYER layer) -> void {
		count += layers[+layer].size();
	});
	return count;
}

ecs::RegisteredSystemsOperatingByLayer::GetEntityLayersMaskSig ecs::RegisteredSystemsOperatingByLayer::GetLayerMaskFunc(SysHash sysHash) const
{
	auto iter{ systemsOperatingByLayer.find(sysHash) };
	if (iter == systemsOperatingByLayer.end())
		return nullptr;
	return iter->second;
}

void ecs::RegisteredSystemsOperatingByLayer::SerializeLayerSettings(Serializer& writer, const std::string& key)
{
	writer.StartObject(key);
	for (const auto& system : systemsOperatingByLayer)
		system.second().MaskSerialize(writer, std::to_string(system.first), entityLayerNames);
	writer.EndObject();
}

void ecs::RegisteredSystemsOperatingByLayer::DeserializeLayerSettings(Deserializer& reader, const std::string& key)
{
	if (!reader.PushAccess(key))
		return;
	for (const auto& system : systemsOperatingByLayer)
		system.second().MaskDeserialize(reader, std::to_string(system.first), entityLayerNames);
	reader.PopAccess();
}

void ecs::RegisteredSystemsOperatingByLayer::RegisterSystem(SysHash sysHash, GetEntityLayersMaskSig func)
{
	systemsOperatingByLayer.try_emplace(sysHash, func);
}
