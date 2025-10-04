#include "MaterialSerialization.h"

using namespace AssetLoading;


void MaterialSerialization::Serialize(Serializer& writer, const ProcessedMaterial& material, std::array<size_t, 5> textureHandles)
{
    writer.Serialize("name", material.name);

    switch (material.alphaMode)
    {
    case AlphaMode::Opaque: writer.Serialize("alphaMode", std::string{ "Opaque" }); break;
    case AlphaMode::Mask:   writer.Serialize("alphaMode", std::string{ "Mask" });   break;
    case AlphaMode::Blend:  writer.Serialize("alphaMode", std::string{ "Blend" });  break;
    }
    writer.Serialize("alphaCutoff", material.alphaCutoff);
    writer.Serialize("metallicFactor", material.metallicFactor);
    writer.Serialize("roughnessFactor", material.roughnessFactor);
    writer.Serialize("normalScale", material.normalScale);
    writer.Serialize("occlusionStrength", material.occlusionStrength);

    std::array<float, 4> baseColor = { material.baseColorFactor.x, material.baseColorFactor.y, material.baseColorFactor.z, material.baseColorFactor.w };
    writer.Serialize("baseColorFactor", baseColor);

    std::array<float, 3> emissive = { material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z };
    writer.Serialize("emissiveFactor", emissive);

    //writer.StartObject();
    std::unordered_map<std::string, size_t> textures;
    textures["baseColor"] = textureHandles[0];
    textures["normal"] = textureHandles[1];
    textures["metallicRoughness"] = textureHandles[2];
    textures["emissive"] = textureHandles[3];
    textures["occlusion"] = textureHandles[4];
    writer.Serialize("textures", textures);
    //writer.EndObject(); // Textures object
}


void MaterialSerialization::Deserialize(Deserializer& reader, ProcessedMaterial& outMaterial)
{


}