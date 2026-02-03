// resource/resource_registry.cpp
#include "resource_registry.h"
#include "resource_manager.h"
#include "asset_loader.h"
#include "VFS/VFS.h"
#include "logging/log.h"

#include <algorithm>
#include <functional>
#include <fstream>
#include <sstream>
#include <set>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace Resource {

// ============================================================================
// CONSTRUCTION / DESTRUCTION
// ============================================================================

ResourceRegistry::ResourceRegistry(ResourceManager* resourceManager)
    : m_resourceManager(resourceManager)
{
    if (!m_resourceManager)
    {
        LOG_ERROR("ResourceRegistry: ResourceManager cannot be null");
    }
}

ResourceRegistry::~ResourceRegistry() = default;

// ============================================================================
// IMPORT API
// ============================================================================

size_t ResourceRegistry::ImportTexture(const std::string& vfsPath)
{
    // Check if already imported
    if (auto it = m_pathToHash.find(vfsPath); it != m_pathToHash.end())
    {
        return it->second;
    }

    // Load texture data
    ProcessedTexture processedTexture = AssetLoader::LoadTexture(vfsPath);
    if (processedTexture.data.empty())
    {
        LOG_ERROR("Failed to load texture: {}", vfsPath);
        return 0;
    }

    // Set the source path for cache key generation
    // This is critical for material texture resolution to find the texture by path
    processedTexture.source = FilePathSource{vfsPath};

    // Upload to GPU
    TextureHandle handle = m_resourceManager->createTexture(std::move(processedTexture));
    if (!handle.isValid())
    {
        LOG_ERROR("Failed to create GPU texture for: {}", vfsPath);
        return 0;
    }

    // Generate hash and register
    size_t hash = GenerateHash(vfsPath);
    RegisterResource(hash, vfsPath, ResourceType::Texture);

    // Store handle
    m_hashToEntry[hash].textureHandle = handle;

    LOG_INFO("Imported texture: {} (hash: {})", vfsPath, hash);
    return hash;
}

size_t ResourceRegistry::ImportMesh(const std::string& vfsPath)
{
    // Check if already imported
    if (auto it = m_pathToHash.find(vfsPath); it != m_pathToHash.end())
    {
        return it->second;
    }

    // Load mesh data
    ProcessedMesh processedMesh = AssetLoader::LoadMesh(vfsPath);
    if (processedMesh.vertices.empty())
    {
        LOG_ERROR("Failed to load mesh: {}", vfsPath);
        return 0;
    }

    // Upload to GPU
    MeshHandle handle = m_resourceManager->createMesh(processedMesh);
    if (!handle.isValid())
    {
        LOG_ERROR("Failed to create GPU mesh for: {}", vfsPath);
        return 0;
    }

    // Generate hash and register
    size_t hash = GenerateHash(vfsPath);
    RegisterResource(hash, vfsPath, ResourceType::Mesh);

    // Store handle
    m_hashToEntry[hash].meshHandle = handle;

    LOG_INFO("Imported mesh: {} (hash: {})", vfsPath, hash);
    return hash;
}

size_t ResourceRegistry::ImportMaterial(const std::string& vfsPath)
{
    // Check if already imported
    if (auto it = m_pathToHash.find(vfsPath); it != m_pathToHash.end())
    {
        return it->second;
    }

    // Load material data
    ProcessedMaterial processedMaterial = AssetLoader::LoadMaterial(vfsPath);

    // Validate material data
    if (!processedMaterial.isValid())
    {
        LOG_WARNING("Material '{}' has invalid PBR values - clamping to valid range", vfsPath);
        // Clamp to valid ranges instead of rejecting
        processedMaterial.metallicFactor = std::clamp(processedMaterial.metallicFactor, 0.0f, 1.0f);
        processedMaterial.roughnessFactor = std::clamp(processedMaterial.roughnessFactor, 0.0f, 1.0f);
        processedMaterial.alphaCutoff = std::clamp(processedMaterial.alphaCutoff, 0.0f, 1.0f);
        processedMaterial.normalScale = std::max(processedMaterial.normalScale, 0.0f);
        processedMaterial.occlusionStrength = std::clamp(processedMaterial.occlusionStrength, 0.0f, 1.0f);
    }

    // Import all textures referenced by the material BEFORE creating the material
    // This ensures textures are in the cache when createMaterial tries to resolve them
    auto importTextureIfNeeded = [this](const MaterialTexture& matTex) {
        if (matTex.hasTexture()) {
            if (auto* pathSource = std::get_if<FilePathSource>(&matTex.source)) {
                if (!pathSource->path.empty()) {
                    LOG_DEBUG("Material references texture: {}", pathSource->path);
                    ImportTexture(pathSource->path);
                }
            }
        }
    };

    importTextureIfNeeded(processedMaterial.baseColorTexture);
    importTextureIfNeeded(processedMaterial.metallicRoughnessTexture);
    importTextureIfNeeded(processedMaterial.normalTexture);
    importTextureIfNeeded(processedMaterial.emissiveTexture);
    importTextureIfNeeded(processedMaterial.occlusionTexture);

    // Upload to GPU (textures are now in cache)
    MaterialHandle handle = m_resourceManager->createMaterial(processedMaterial);
    if (!handle.isValid())
    {
        LOG_ERROR("Failed to create GPU material for: {}", vfsPath);
        return 0;
    }

    // Generate hash and register
    size_t hash = GenerateHash(vfsPath);
    RegisterResource(hash, vfsPath, ResourceType::Material);

    // Store handle
    m_hashToEntry[hash].materialHandle = handle;

    LOG_INFO("Imported material: {} (hash: {})", vfsPath, hash);
    return hash;
}

size_t ResourceRegistry::ImportAnimation(const std::string& vfsPath)
{
    // Check if already imported
    if (auto it = m_pathToHash.find(vfsPath); it != m_pathToHash.end())
    {
        return it->second;
    }

    // Load animation data
    ProcessedAnimationClip processedClip = AssetLoader::LoadAnimation(vfsPath);
    if (processedClip.skeletalChannels.empty() && processedClip.morphChannels.empty())
    {
        LOG_ERROR("Failed to load animation: {}", vfsPath);
        return 0;
    }

    // Create animation clip
    ClipId clipId = m_resourceManager->createClip(processedClip);
    if (clipId == INVALID_CLIP_ID)
    {
        LOG_ERROR("Failed to create GPU animation clip for: {}", vfsPath);
        return 0;
    }

    // Generate hash and register
    size_t hash = GenerateHash(vfsPath);
    RegisterResource(hash, vfsPath, ResourceType::Animation);

    // Store clip ID
    m_hashToEntry[hash].clipId = clipId;

    LOG_INFO("Imported animation: {} (hash: {})", vfsPath, hash);
    return hash;
}

ResourceRegistry::ModelImportResult ResourceRegistry::ImportModel(const std::string& vfsPath, const std::string& materialsBasePath)
{
    ModelImportResult result;
    result.sourcePath = vfsPath;
    result.success = false;

    // Load the complete model with all submeshes
    ProcessedModel model = AssetLoader::LoadModel(vfsPath, materialsBasePath);
    if (model.submeshes.empty())
    {
        LOG_ERROR("Failed to load model: {}", vfsPath);
        return result;
    }

    result.hasSkeleton = model.hasSkeleton;
    result.hasMorphs = model.hasMorphs;
    result.submeshes.reserve(model.submeshes.size());

    // Import each submesh
    for (size_t i = 0; i < model.submeshes.size(); ++i)
    {
        const auto& modelSubmesh = model.submeshes[i];
        ModelImportResult::SubmeshEntry entry;

        // Create a unique path for this submesh: "models/char.mesh#SubmeshName"
        entry.meshPath = vfsPath + "#" + modelSubmesh.mesh.name;
        entry.materialPath = modelSubmesh.materialPath;

        // Check if this submesh was already imported
        if (auto it = m_pathToHash.find(entry.meshPath); it != m_pathToHash.end())
        {
            entry.meshHash = it->second;
            LOG_DEBUG("Submesh already imported: {}", entry.meshPath);
        }
        else
        {
            // Upload mesh to GPU
            MeshHandle meshHandle = m_resourceManager->createMesh(modelSubmesh.mesh);
            if (!meshHandle.isValid())
            {
                LOG_ERROR("Failed to create GPU mesh for submesh: {}", entry.meshPath);
                continue;
            }

            // Generate hash and register
            entry.meshHash = GenerateHash(entry.meshPath);
            RegisterResource(entry.meshHash, entry.meshPath, ResourceType::Mesh);
            m_hashToEntry[entry.meshHash].meshHandle = meshHandle;

            LOG_DEBUG("Imported submesh: {} (hash: {})", entry.meshPath, entry.meshHash);
        }

        // Import material if specified
        if (!entry.materialPath.empty())
        {
            entry.materialHash = ImportMaterial(entry.materialPath);
            if (entry.materialHash == 0)
            {
                LOG_WARNING("Failed to import material for submesh {}: {}", entry.meshPath, entry.materialPath);
            }
        }

        // Store the submesh -> material association for later lookup (even if empty)
        m_submeshToMaterial[entry.meshPath] = entry.materialPath;

        result.submeshes.push_back(std::move(entry));
    }

    result.success = !result.submeshes.empty();
    LOG_INFO("Imported model '{}' - {} submeshes", vfsPath, result.submeshes.size());
    return result;
}

ResourceRegistry::MeshFileMetadata ResourceRegistry::QueryMeshFileInfo(const std::string& vfsPath) const
{
    MeshFileMetadata metadata;

    // Use AssetLoader to query the file info without loading vertex data
    Resource::MeshFileInfo info = AssetLoader::QueryMeshFileInfo(vfsPath);

    metadata.sourcePath = info.sourcePath;
    metadata.hasSkeleton = info.hasSkeleton;
    metadata.hasMorphs = info.hasMorphs;
    metadata.totalVertices = info.totalVertices;
    metadata.totalIndices = info.totalIndices;

    metadata.submeshes.reserve(info.submeshes.size());
    for (const auto& sub : info.submeshes)
    {
        MeshFileMetadata::SubmeshInfo subInfo;
        subInfo.name = sub.name;
        subInfo.materialName = sub.materialName;
        subInfo.vertexCount = sub.vertexCount;
        subInfo.indexCount = sub.indexCount;
        metadata.submeshes.push_back(std::move(subInfo));
    }

    return metadata;
}

// ============================================================================
// LOOKUP API - BY HASH
// ============================================================================

TextureHandle ResourceRegistry::GetTextureByHash(size_t hash) const
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return TextureHandle{}; // Invalid handle
    }

    if (it->second.type != ResourceType::Texture)
    {
        LOG_WARNING("Resource hash {} is not a texture", hash);
        return TextureHandle{};
    }

    return it->second.textureHandle;
}

MeshHandle ResourceRegistry::GetMeshByHash(size_t hash) const
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return MeshHandle{}; // Invalid handle
    }

    if (it->second.type != ResourceType::Mesh)
    {
        LOG_WARNING("Resource hash {} is not a mesh", hash);
        return MeshHandle{};
    }

    return it->second.meshHandle;
}

MaterialHandle ResourceRegistry::GetMaterialByHash(size_t hash) const
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return MaterialHandle{}; // Invalid handle
    }

    if (it->second.type != ResourceType::Material)
    {
        LOG_WARNING("Resource hash {} is not a material", hash);
        return MaterialHandle{};
    }

    return it->second.materialHandle;
}

ClipId ResourceRegistry::GetAnimationByHash(size_t hash) const
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return INVALID_CLIP_ID;
    }

    if (it->second.type != ResourceType::Animation)
    {
        LOG_WARNING("Resource hash {} is not an animation", hash);
        return INVALID_CLIP_ID;
    }

    return it->second.clipId;
}

// ============================================================================
// LOOKUP API - BY PATH
// ============================================================================

TextureHandle ResourceRegistry::GetTextureByPath(const std::string& vfsPath) const
{
    auto it = m_pathToHash.find(vfsPath);
    if (it == m_pathToHash.end())
    {
        return TextureHandle{}; // Not found
    }

    return GetTextureByHash(it->second);
}

MeshHandle ResourceRegistry::GetMeshByPath(const std::string& vfsPath) const
{
    auto it = m_pathToHash.find(vfsPath);
    if (it == m_pathToHash.end())
    {
        return MeshHandle{}; // Not found
    }

    return GetMeshByHash(it->second);
}

MaterialHandle ResourceRegistry::GetMaterialByPath(const std::string& vfsPath) const
{
    auto it = m_pathToHash.find(vfsPath);
    if (it == m_pathToHash.end())
    {
        return MaterialHandle{}; // Not found
    }

    return GetMaterialByHash(it->second);
}

ClipId ResourceRegistry::GetAnimationByPath(const std::string& vfsPath) const
{
    auto it = m_pathToHash.find(vfsPath);
    if (it == m_pathToHash.end())
    {
        return INVALID_CLIP_ID; // Not found
    }

    return GetAnimationByHash(it->second);
}

// ============================================================================
// FILE PATH TRACKING
// ============================================================================

std::string ResourceRegistry::GetFilePath(size_t hash) const
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return "";
    }

    return it->second.filePath;
}

size_t ResourceRegistry::GetHashFromPath(const std::string& vfsPath) const
{
    auto it = m_pathToHash.find(vfsPath);
    if (it == m_pathToHash.end())
    {
        return 0;
    }

    return it->second;
}

bool ResourceRegistry::HasResource(size_t hash) const
{
    return m_hashToEntry.find(hash) != m_hashToEntry.end();
}

// ============================================================================
// RESOURCE MANAGEMENT
// ============================================================================

bool ResourceRegistry::UnloadResource(size_t hash)
{
    auto it = m_hashToEntry.find(hash);
    if (it == m_hashToEntry.end())
    {
        return false;
    }

    // Free GPU resource based on type
    switch (it->second.type)
    {
    case ResourceType::Texture:
        m_resourceManager->freeTexture(it->second.textureHandle);
        break;
    case ResourceType::Mesh:
        m_resourceManager->freeMesh(it->second.meshHandle);
        break;
    case ResourceType::Material:
        m_resourceManager->freeMaterial(it->second.materialHandle);
        break;
    case ResourceType::Animation:
        m_resourceManager->freeClip(it->second.clipId);
        break;
    }

    // Remove from tracking
    std::string filePath = it->second.filePath;
    m_hashToEntry.erase(it);
    m_pathToHash.erase(filePath);

    // Remove name mapping if exists
    if (auto nameIt = m_hashToName.find(hash); nameIt != m_hashToName.end())
    {
        m_nameToHash.erase(nameIt->second);
        m_hashToName.erase(nameIt);
    }

    LOG_INFO("Unloaded resource: {} (hash: {})", filePath, hash);
    return true;
}

ResourceRegistry::Stats ResourceRegistry::GetStats() const
{
    Stats stats{};

    for (const auto& [hash, entry] : m_hashToEntry)
    {
        switch (entry.type)
        {
        case ResourceType::Texture:
            stats.numTextures++;
            break;
        case ResourceType::Mesh:
            stats.numMeshes++;
            break;
        case ResourceType::Material:
            stats.numMaterials++;
            break;
        case ResourceType::Animation:
            stats.numAnimations++;
            break;
        }
    }

    return stats;
}

// ============================================================================
// ENUMERATION API
// ============================================================================

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllResources() const
{
    std::vector<ResourceInfo> result;
    result.reserve(m_hashToEntry.size());

    for (const auto& [hash, entry] : m_hashToEntry)
    {
        ResourceInfo info{};
        info.hash = hash;
        info.filePath = entry.filePath;
        info.type = entry.type;

        // Look up human-readable name if set
        if (auto nameIt = m_hashToName.find(hash); nameIt != m_hashToName.end())
        {
            info.name = nameIt->second;
        }

        result.push_back(std::move(info));
    }

    return result;
}

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllResourcesOfType(ResourceType type) const
{
    std::vector<ResourceInfo> result;

    for (const auto& [hash, entry] : m_hashToEntry)
    {
        if (entry.type != type)
            continue;

        ResourceInfo info{};
        info.hash = hash;
        info.filePath = entry.filePath;
        info.type = entry.type;

        if (auto nameIt = m_hashToName.find(hash); nameIt != m_hashToName.end())
        {
            info.name = nameIt->second;
        }

        result.push_back(std::move(info));
    }

    return result;
}

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllTextures() const
{
    return GetAllResourcesOfType(ResourceType::Texture);
}

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllMeshes() const
{
    return GetAllResourcesOfType(ResourceType::Mesh);
}

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllMaterials() const
{
    return GetAllResourcesOfType(ResourceType::Material);
}

std::vector<ResourceRegistry::ResourceInfo> ResourceRegistry::GetAllAnimations() const
{
    return GetAllResourcesOfType(ResourceType::Animation);
}

// ============================================================================
// NAME LIST API
// ============================================================================

std::vector<std::string> ResourceRegistry::GetAllMeshNames() const
{
    std::vector<std::string> names;
    for (const auto& [hash, entry] : m_hashToEntry)
    {
        if (entry.type == ResourceType::Mesh)
        {
            names.push_back(entry.filePath);
        }
    }
    return names;
}

std::vector<std::string> ResourceRegistry::GetAllTextureNames() const
{
    std::vector<std::string> names;
    for (const auto& [hash, entry] : m_hashToEntry)
    {
        if (entry.type == ResourceType::Texture)
        {
            names.push_back(entry.filePath);
        }
    }
    return names;
}

std::vector<std::string> ResourceRegistry::GetAllMaterialNames() const
{
    std::vector<std::string> names;
    for (const auto& [hash, entry] : m_hashToEntry)
    {
        if (entry.type == ResourceType::Material)
        {
            names.push_back(entry.filePath);
        }
    }
    return names;
}

std::vector<std::string> ResourceRegistry::GetAllAnimationNames() const
{
    std::vector<std::string> names;
    for (const auto& [hash, entry] : m_hashToEntry)
    {
        if (entry.type == ResourceType::Animation)
        {
            names.push_back(entry.filePath);
        }
    }
    return names;
}

// ============================================================================
// RESOURCE NAMING API
// ============================================================================

bool ResourceRegistry::SetResourceName(size_t hash, const std::string& name)
{
    // Check if resource exists
    if (m_hashToEntry.find(hash) == m_hashToEntry.end())
    {
        LOG_WARNING("Cannot set name for non-existent resource hash: {}", hash);
        return false;
    }

    // Remove old name mapping if exists
    if (auto oldNameIt = m_hashToName.find(hash); oldNameIt != m_hashToName.end())
    {
        m_nameToHash.erase(oldNameIt->second);
    }

    // Check if name is already in use by another resource
    if (auto existingIt = m_nameToHash.find(name); existingIt != m_nameToHash.end())
    {
        if (existingIt->second != hash)
        {
            LOG_WARNING("Resource name '{}' is already in use by hash {}", name, existingIt->second);
            return false;
        }
    }

    // Set new name
    m_hashToName[hash] = name;
    m_nameToHash[name] = hash;

    LOG_DEBUG("Set resource name: {} -> hash {}", name, hash);
    return true;
}

std::string ResourceRegistry::GetResourceName(size_t hash) const
{
    if (auto it = m_hashToName.find(hash); it != m_hashToName.end())
    {
        return it->second;
    }
    return "";
}

size_t ResourceRegistry::GetHashByName(const std::string& name) const
{
    if (auto it = m_nameToHash.find(name); it != m_nameToHash.end())
    {
        return it->second;
    }
    return 0;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

size_t ResourceRegistry::GenerateHash(const std::string& vfsPath)
{
    // Use std::hash for now
    // Could be upgraded to a better hash function (e.g., FNV-1a, xxHash)
    return std::hash<std::string>{}(vfsPath);
}

void ResourceRegistry::RegisterResource(size_t hash, const std::string& vfsPath, ResourceType type)
{
    ResourceEntry entry{};
    entry.type = type;
    entry.filePath = vfsPath;

    m_hashToEntry[hash] = entry;
    m_pathToHash[vfsPath] = hash;
}

// ============================================================================
// PERSISTENCE API
// ============================================================================

bool ResourceRegistry::SaveToFile(const std::string& filePath) const
{
    rapidjson::Document doc;
    doc.SetObject();
    auto& allocator = doc.GetAllocator();

    rapidjson::Value resources(rapidjson::kArrayType);

    for (const auto& [hash, entry] : m_hashToEntry)
    {
        rapidjson::Value resourceObj(rapidjson::kObjectType);

        // Store file path (this is the VFS path used for re-import)
        resourceObj.AddMember("path",
            rapidjson::Value(entry.filePath.c_str(), allocator),
            allocator);

        // Store type as string for readability
        const char* typeStr = "unknown";
        switch (entry.type)
        {
        case ResourceType::Texture:   typeStr = "texture"; break;
        case ResourceType::Mesh:      typeStr = "mesh"; break;
        case ResourceType::Material:  typeStr = "material"; break;
        case ResourceType::Animation: typeStr = "animation"; break;
        }
        resourceObj.AddMember("type",
            rapidjson::Value(typeStr, allocator),
            allocator);

        // Store human-readable name if set
        if (auto nameIt = m_hashToName.find(hash); nameIt != m_hashToName.end())
        {
            resourceObj.AddMember("name",
                rapidjson::Value(nameIt->second.c_str(), allocator),
                allocator);
        }

        resources.PushBack(resourceObj, allocator);
    }

    doc.AddMember("resources", resources, allocator);

    // Write to file
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file for writing: {}", filePath);
        return false;
    }

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    file << buffer.GetString();
    file.close();

    LOG_INFO("Saved {} resources to {}", m_hashToEntry.size(), filePath);
    return true;
}

int ResourceRegistry::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        LOG_WARNING("Registry file not found: {}", filePath);
        return -1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();
    file.close();

    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError() || !doc.IsObject())
    {
        LOG_ERROR("Failed to parse registry JSON: {}", filePath);
        return -1;
    }

    if (!doc.HasMember("resources") || !doc["resources"].IsArray())
    {
        LOG_ERROR("Invalid registry format: missing 'resources' array");
        return -1;
    }

    int loadedCount = 0;
    const auto& resources = doc["resources"].GetArray();

    for (const auto& resourceObj : resources)
    {
        if (!resourceObj.IsObject() || !resourceObj.HasMember("path") || !resourceObj.HasMember("type"))
        {
            LOG_WARNING("Skipping invalid resource entry");
            continue;
        }

        std::string vfsPath = resourceObj["path"].GetString();
        std::string typeStr = resourceObj["type"].GetString();

        // Re-import the resource based on type
        size_t hash = 0;
        if (typeStr == "texture")
        {
            hash = ImportTexture(vfsPath);
        }
        else if (typeStr == "mesh")
        {
            hash = ImportMesh(vfsPath);
        }
        else if (typeStr == "material")
        {
            hash = ImportMaterial(vfsPath);
        }
        else if (typeStr == "animation")
        {
            hash = ImportAnimation(vfsPath);
        }
        else
        {
            LOG_WARNING("Unknown resource type '{}' for path: {}", typeStr, vfsPath);
            continue;
        }

        if (hash != 0)
        {
            // Restore human-readable name if it was saved
            if (resourceObj.HasMember("name") && resourceObj["name"].IsString())
            {
                SetResourceName(hash, resourceObj["name"].GetString());
            }
            loadedCount++;
        }
        else
        {
            LOG_WARNING("Failed to re-import resource: {}", vfsPath);
        }
    }

    LOG_INFO("Loaded {} of {} resources from {}", loadedCount, resources.Size(), filePath);
    return loadedCount;
}

// ============================================================================
// SUBMESH/SOURCE TRACKING API
// ============================================================================

std::string ResourceRegistry::GetSubmeshMaterial(const std::string& submeshPath) const
{
    auto it = m_submeshToMaterial.find(submeshPath);
    if (it != m_submeshToMaterial.end())
    {
        return it->second;
    }
    return "";
}

std::vector<std::pair<std::string, std::string>> ResourceRegistry::GetModelSubmeshes(const std::string& parentMeshPath) const
{
    std::vector<std::pair<std::string, std::string>> result;
    std::string prefix = parentMeshPath + "#";

    for (const auto& [submeshPath, materialPath] : m_submeshToMaterial)
    {
        if (submeshPath.rfind(prefix, 0) == 0)  // starts with prefix
        {
            result.emplace_back(submeshPath, materialPath);
        }
    }

    // Also check if the parent path itself is a single-mesh model (no # in path)
    if (result.empty())
    {
        auto it = m_submeshToMaterial.find(parentMeshPath);
        if (it != m_submeshToMaterial.end())
        {
            result.emplace_back(it->first, it->second);
        }
    }

    return result;
}

std::string ResourceRegistry::GetAssetSource(const std::string& assetPath) const
{
    auto it = m_assetToSource.find(assetPath);
    if (it != m_assetToSource.end())
    {
        return it->second;
    }
    return "";
}

std::vector<std::string> ResourceRegistry::GetAssetsFromSource(const std::string& sourcePath) const
{
    std::vector<std::string> result;
    for (const auto& [assetPath, source] : m_assetToSource)
    {
        if (source == sourcePath)
        {
            result.push_back(assetPath);
        }
    }
    return result;
}

void ResourceRegistry::SetAssetSource(const std::string& assetPath, const std::string& sourcePath)
{
    m_assetToSource[assetPath] = sourcePath;
}

std::vector<std::string> ResourceRegistry::GetAllSourceFiles() const
{
    std::set<std::string> uniqueSources;
    for (const auto& [assetPath, source] : m_assetToSource)
    {
        if (!source.empty())
        {
            uniqueSources.insert(source);
        }
    }
    return std::vector<std::string>(uniqueSources.begin(), uniqueSources.end());
}

} // namespace Resource
