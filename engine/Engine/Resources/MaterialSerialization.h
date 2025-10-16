#pragma once
#include "Serializer.h"
#include "processed_assets.h"

#include <array>

class MaterialSerialization
{
public:
    static void Serialize(Serializer& writer, const AssetLoading::ProcessedMaterial& material, std::array<size_t, 5> textureHandles);
    static void Deserialize(Deserializer& reader, AssetLoading::ProcessedMaterial& out_material);
};