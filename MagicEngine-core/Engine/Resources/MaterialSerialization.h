#pragma once
#include "Utilities/Serializer.h"
#include "resource/processed_assets.h"

#include <array>

class MaterialSerialization
{
public:
    static void Serialize(Serializer& writer, const Resource::ProcessedMaterial& material, std::array<size_t, 5> textureHandles);
    static void Deserialize(Deserializer& reader, Resource::ProcessedMaterial& out_material);
};