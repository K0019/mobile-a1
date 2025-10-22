#include "Engine/Resources/Importers/ResourceFiletypeImporterMaterial.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Resources/ResourceImporter.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "tools/assets/io/import_config.h"
#include "Engine/Resources/MaterialSerialization.h"
#include "GameSettings.h"

using namespace Resource;

namespace
{
    void populateTexturesVector(ProcessedMaterial& material)
    {
        material.textures.clear();

        // Helper to add valid texture sources
        auto addIfValid = [&](const MaterialTexture& matTex)
            {
                if (matTex.hasTexture())
                {
                    material.textures.push_back(matTex.source);
                }
            };
        addIfValid(material.baseColorTexture);
        addIfValid(material.metallicRoughnessTexture);
        addIfValid(material.normalTexture);
        addIfValid(material.emissiveTexture);
        addIfValid(material.occlusionTexture);
    }
}

bool ResourceFiletypeImporterMaterial::Import(const std::filesystem::path& relativeFilepath)
{
    // Load the file
    std::filesystem::path fullPath = GetExeRelativeFilepath(relativeFilepath);
    ProcessedMaterial material;

    Deserializer reader{ fullPath.string()};
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open .material file ";
        return false;
    }
    MaterialSerialization::Deserialize(reader, material);
    populateTexturesVector(material);

    //Ensure that textures are loaded before we link them with the materials
    for (const auto& textureDataSource : material.textures)
    {
        if (const auto* filePath = std::get_if<FilePathSource>(&textureDataSource))
        {
            ResourceImporter::Import(filePath->path);
        }
    }

    // Create material handle
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    MaterialHandle materialHandle{ graphicsAssetSystem.createMaterial(material) };
    graphicsAssetSystem.FlushUploads();

    // Create resource entry
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMaterial>(relativeFilepath, 1) };

    // Assign resource to material handle
    ST<MagicResourceManager>::Get()->INTERNAL_GetMaterials().INTERNAL_GetResource(fileEntry->associatedResources[0].hashes[0], true)->handle = materialHandle;

	return true;
}