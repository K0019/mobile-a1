#include "ResourceFiletypeImporterMaterial.h"
#include "ResourceTypesGraphics.h"
#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "GraphicsAPI.h"
#include "import_config.h"
#include "MeshFileStructure.h"
#include "MaterialSerialization.h"

using namespace AssetLoading;

namespace internal
{
    void DeserializeMaterial(Deserializer& reader, Material* outMaterial)
    {
        reader.DeserializeVar("name", &outMaterial->name);

        // textures: unordered_map<string,string>

        // params: unordered_map<string,float>
    }

}


bool ResourceFiletypeImporterMaterial::Import(const std::filesystem::path& relativeFilepath)
{
    ProcessedMaterial material;

    Deserializer reader{ relativeFilepath.string()};
    MaterialSerialization::Deserialize(reader, material);

    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open .material file ";
        return false;
    }

	return true;
}

const ResourceFilepaths::FileEntry* ResourceFiletypeImporterMaterial::CreateNewFileEntry(const std::filesystem::path& relativeFilepath)
{
	return nullptr;
}
