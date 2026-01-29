// resource/resource_registry.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>

#include "resource_handle.h"
#include "resource_types.h"
#include "animation_ids.h"

namespace Resource {

// Bring handle types into Resource namespace (they're defined globally in resource_types.h)
using ::TextureHandle;
using ::MeshHandle;
using ::MaterialHandle;

// Forward declarations
class ResourceManager;
class AssetLoader;

/**
 * @brief High-level resource registry with hash-based lookup and file tracking
 *
 * ResourceRegistry provides a game-facing API for resource management, built on top of:
 * - AssetLoader: File I/O via VFS
 * - ResourceManager: GPU memory management
 *
 * Features:
 * - Hash-based resource identification (for serialization)
 * - File path tracking (hash ↔ filepath bidirectional mapping)
 * - Import API for loading assets from files
 * - Query API for retrieving resources by hash or path
 *
 * Architecture:
 * ```
 * Game Code
 *     ↓
 * ResourceRegistry (this class) - Hash API, file tracking
 *     ↓
 * AssetLoader - File parsing via VFS
 *     ↓
 * ResourceManager - GPU upload, pooling, caching
 * ```
 *
 * Usage:
 * ```cpp
 * ResourceRegistry registry(resourceManager);
 *
 * // Import from file
 * size_t hash = registry.ImportTexture("textures/foo.ktx2");
 *
 * // Lookup by hash
 * TextureHandle handle = registry.GetTextureByHash(hash);
 *
 * // Lookup by path
 * TextureHandle handle2 = registry.GetTextureByPath("textures/foo.ktx2");
 * ```
 */
class ResourceRegistry
{
public:
    /**
     * @brief Construct a ResourceRegistry
     * @param resourceManager Reference to the GPU resource manager (must outlive this registry)
     */
    explicit ResourceRegistry(ResourceManager* resourceManager);
    ~ResourceRegistry();

    // Prevent copying (registry owns mappings)
    ResourceRegistry(const ResourceRegistry&) = delete;
    ResourceRegistry& operator=(const ResourceRegistry&) = delete;

    // ========================================================================
    // IMPORT API - Load assets from files
    // ========================================================================

    /**
     * @brief Import a texture from a file
     * @param vfsPath Virtual file system path (e.g., "textures/foo.ktx2")
     * @return Hash of the imported resource, or 0 on failure
     */
    size_t ImportTexture(const std::string& vfsPath);

    /**
     * @brief Import a mesh from a file
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @return Hash of the imported resource, or 0 on failure
     */
    size_t ImportMesh(const std::string& vfsPath);

    /**
     * @brief Import a material from a file
     * @param vfsPath Virtual file system path (e.g., "materials/metal.material")
     * @return Hash of the imported resource, or 0 on failure
     */
    size_t ImportMaterial(const std::string& vfsPath);

    /**
     * @brief Import an animation from a file
     * @param vfsPath Virtual file system path (e.g., "animations/run.anim")
     * @return Hash of the imported resource, or 0 on failure
     */
    size_t ImportAnimation(const std::string& vfsPath);

    /**
     * @brief Result of importing a model with multiple submeshes
     * Contains the mesh/material pairs needed to populate MeshRendererComponent
     */
    struct ModelImportResult
    {
        struct SubmeshEntry
        {
            std::string meshPath;       // VFS path to use for lookup (e.g., "models/char.mesh#Body")
            std::string materialPath;   // VFS path to material file
            size_t meshHash = 0;        // Hash for mesh lookup
            size_t materialHash = 0;    // Hash for material lookup
        };

        std::string sourcePath;                 // Original .mesh file path
        std::vector<SubmeshEntry> submeshes;    // All submesh-material pairs
        bool hasSkeleton = false;
        bool hasMorphs = false;
        bool success = false;
    };

    /**
     * @brief Import a model file with all submeshes and their materials
     * Use this for complex models with multiple mesh+material pairs.
     * Each submesh is registered separately with a path like "models/char.mesh#SubmeshName"
     *
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @param materialsBasePath Base path for resolving material references (e.g., "materials/")
     * @return ModelImportResult with all submesh-material pairs for MeshRendererComponent
     *
     * Example usage:
     * ```cpp
     * auto result = registry.ImportModel("models/character.mesh", "materials/");
     * if (result.success) {
     *     MeshRendererComponent renderer;
     *     for (const auto& sub : result.submeshes) {
     *         renderer.AddEntry(sub.meshPath, sub.materialPath);
     *     }
     * }
     * ```
     */
    ModelImportResult ImportModel(const std::string& vfsPath, const std::string& materialsBasePath = "");

    /**
     * @brief Query metadata about a mesh file without full import
     * Useful for Editor to display submesh information in asset browser
     *
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @return MeshFileInfo with submesh names, material names, counts
     */
    struct MeshFileMetadata
    {
        struct SubmeshInfo
        {
            std::string name;
            std::string materialName;
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
        };

        std::string sourcePath;
        std::vector<SubmeshInfo> submeshes;
        bool hasSkeleton = false;
        bool hasMorphs = false;
        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
    };

    MeshFileMetadata QueryMeshFileInfo(const std::string& vfsPath) const;

    // ========================================================================
    // LOOKUP API - Retrieve resources by hash or path
    // ========================================================================

    /**
     * @brief Get texture handle by hash
     * @param hash Resource hash
     * @return Texture handle, or invalid handle if not found
     */
    TextureHandle GetTextureByHash(size_t hash) const;

    /**
     * @brief Get texture handle by file path
     * @param vfsPath Virtual file system path
     * @return Texture handle, or invalid handle if not found
     */
    TextureHandle GetTextureByPath(const std::string& vfsPath) const;

    /**
     * @brief Get mesh handle by hash
     * @param hash Resource hash
     * @return Mesh handle, or invalid handle if not found
     */
    MeshHandle GetMeshByHash(size_t hash) const;

    /**
     * @brief Get mesh handle by file path
     * @param vfsPath Virtual file system path
     * @return Mesh handle, or invalid handle if not found
     */
    MeshHandle GetMeshByPath(const std::string& vfsPath) const;

    /**
     * @brief Get material handle by hash
     * @param hash Resource hash
     * @return Material handle, or invalid handle if not found
     */
    MaterialHandle GetMaterialByHash(size_t hash) const;

    /**
     * @brief Get material handle by file path
     * @param vfsPath Virtual file system path
     * @return Material handle, or invalid handle if not found
     */
    MaterialHandle GetMaterialByPath(const std::string& vfsPath) const;

    /**
     * @brief Get animation clip ID by hash
     * @param hash Resource hash
     * @return Clip ID, or INVALID_CLIP_ID if not found
     */
    ClipId GetAnimationByHash(size_t hash) const;

    /**
     * @brief Get animation clip ID by file path
     * @param vfsPath Virtual file system path
     * @return Clip ID, or INVALID_CLIP_ID if not found
     */
    ClipId GetAnimationByPath(const std::string& vfsPath) const;

    // ========================================================================
    // FILE PATH TRACKING
    // ========================================================================

    /**
     * @brief Get the file path associated with a resource hash
     * @param hash Resource hash
     * @return File path, or empty string if not found
     */
    std::string GetFilePath(size_t hash) const;

    /**
     * @brief Get the resource hash for a file path
     * @param vfsPath Virtual file system path
     * @return Resource hash, or 0 if not found
     */
    size_t GetHashFromPath(const std::string& vfsPath) const;

    /**
     * @brief Check if a resource with the given hash exists
     * @param hash Resource hash
     * @return true if resource exists, false otherwise
     */
    bool HasResource(size_t hash) const;

    // ========================================================================
    // RESOURCE MANAGEMENT
    // ========================================================================

    /**
     * @brief Unload a resource by hash
     * @param hash Resource hash
     * @return true if resource was unloaded, false if not found
     */
    bool UnloadResource(size_t hash);

    // ========================================================================
    // PERSISTENCE API
    // ========================================================================

    /**
     * @brief Save the registry state to a JSON file
     * @param filePath Path to the JSON file (physical path, not VFS)
     * @return true if save succeeded
     */
    bool SaveToFile(const std::string& filePath) const;

    /**
     * @brief Load and re-import resources from a JSON file
     * @param filePath Path to the JSON file (physical path, not VFS)
     * @return Number of resources successfully loaded, or -1 on error
     */
    int LoadFromFile(const std::string& filePath);

    /**
     * @brief Get statistics about loaded resources
     */
    struct Stats {
        size_t numTextures = 0;
        size_t numMeshes = 0;
        size_t numMaterials = 0;
        size_t numAnimations = 0;
    };
    Stats GetStats() const;

    // ========================================================================
    // RESOURCE TYPE ENUM
    // ========================================================================

    enum class ResourceType {
        Texture,
        Mesh,
        Material,
        Animation
    };

    // ========================================================================
    // ENUMERATION API - Get all resources
    // ========================================================================

    /**
     * @brief Information about a single resource entry
     */
    struct ResourceInfo {
        size_t hash;
        std::string filePath;
        std::string name;  // Human-readable name (empty if not set)
        ResourceType type;
    };

    /**
     * @brief Get all registered resources
     * @return Vector of ResourceInfo for all resources
     */
    std::vector<ResourceInfo> GetAllResources() const;

    /**
     * @brief Get all resources of a specific type
     * @param type The resource type to filter by
     * @return Vector of ResourceInfo for resources of that type
     */
    std::vector<ResourceInfo> GetAllResourcesOfType(ResourceType type) const;

    /**
     * @brief Get all texture resources
     * @return Vector of (hash, filePath, name) tuples
     */
    std::vector<ResourceInfo> GetAllTextures() const;

    /**
     * @brief Get all mesh resources
     * @return Vector of (hash, filePath, name) tuples
     */
    std::vector<ResourceInfo> GetAllMeshes() const;

    /**
     * @brief Get all material resources
     * @return Vector of (hash, filePath, name) tuples
     */
    std::vector<ResourceInfo> GetAllMaterials() const;

    /**
     * @brief Get all animation resources
     * @return Vector of (hash, filePath, name) tuples
     */
    std::vector<ResourceInfo> GetAllAnimations() const;

    // ========================================================================
    // NAME LIST API - Returns display-ready names (file paths for editor)
    // ========================================================================

    /**
     * @brief Get all mesh file paths (for editor display)
     * @return Vector of VFS paths for all meshes
     */
    std::vector<std::string> GetAllMeshNames() const;

    /**
     * @brief Get all texture file paths (for editor display)
     * @return Vector of VFS paths for all textures
     */
    std::vector<std::string> GetAllTextureNames() const;

    /**
     * @brief Get all material file paths (for editor display)
     * @return Vector of VFS paths for all materials
     */
    std::vector<std::string> GetAllMaterialNames() const;

    /**
     * @brief Get all animation file paths (for editor display)
     * @return Vector of VFS paths for all animations
     */
    std::vector<std::string> GetAllAnimationNames() const;

    // ========================================================================
    // RESOURCE NAMING API
    // ========================================================================

    /**
     * @brief Set a human-readable name for a resource
     * @param hash Resource hash
     * @param name Human-readable name (e.g., "player_texture", "enemy_model")
     * @return true if the resource exists and name was set, false otherwise
     */
    bool SetResourceName(size_t hash, const std::string& name);

    /**
     * @brief Get the human-readable name for a resource
     * @param hash Resource hash
     * @return The name, or empty string if not set or resource doesn't exist
     */
    std::string GetResourceName(size_t hash) const;

    /**
     * @brief Find a resource by its human-readable name
     * @param name The name to search for
     * @return Resource hash, or 0 if not found
     */
    size_t GetHashByName(const std::string& name) const;

    // ========================================================================
    // SUBMESH/SOURCE TRACKING API
    // ========================================================================

    /**
     * @brief Get the material path associated with a submesh
     * @param submeshPath The submesh path (e.g., "models/char.mesh#Body")
     * @return Material path, or empty string if not found
     */
    std::string GetSubmeshMaterial(const std::string& submeshPath) const;

    /**
     * @brief Get all submesh paths and their materials for a parent mesh
     * @param parentMeshPath The parent mesh path (e.g., "models/char.mesh")
     * @return Vector of (submeshPath, materialPath) pairs
     */
    std::vector<std::pair<std::string, std::string>> GetModelSubmeshes(const std::string& parentMeshPath) const;

    /**
     * @brief Get the source model path for an asset (texture, animation, etc.)
     * @param assetPath The asset path (e.g., "textures/char_diffuse.ktx2")
     * @return Source model path, or empty string if not found/not from a model
     */
    std::string GetAssetSource(const std::string& assetPath) const;

    /**
     * @brief Get all assets that were imported from a source model
     * @param sourcePath The source model path (e.g., "models/char.mesh")
     * @return Vector of asset paths that came from this source
     */
    std::vector<std::string> GetAssetsFromSource(const std::string& sourcePath) const;

    /**
     * @brief Set the source file for an asset (for grouping in asset browser)
     * @param assetPath The asset path (e.g., "textures/char_diffuse.ktx2")
     * @param sourcePath The source file it came from (e.g., "models/char.fbx")
     */
    void SetAssetSource(const std::string& assetPath, const std::string& sourcePath);

    /**
     * @brief Get all unique source files that have assets
     * @return Vector of unique source file paths
     */
    std::vector<std::string> GetAllSourceFiles() const;

private:
    // ========================================================================
    // INTERNAL TYPES
    // ========================================================================

    struct ResourceEntry {
        ResourceType type;
        std::string filePath;

        // Handle storage (only one will be valid based on type)
        TextureHandle textureHandle;
        MeshHandle meshHandle;
        MaterialHandle materialHandle;
        ClipId clipId = INVALID_CLIP_ID;  // For animations
    };

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /**
     * @brief Generate a hash from a file path
     * Uses std::hash<std::string> for now, could be upgraded to a better hash function
     */
    static size_t GenerateHash(const std::string& vfsPath);

    /**
     * @brief Register a resource in the tracking system
     */
    void RegisterResource(size_t hash, const std::string& vfsPath, ResourceType type);

    // ========================================================================
    // MEMBER DATA
    // ========================================================================

    ResourceManager* m_resourceManager;  // Not owned

    // Bidirectional mappings
    std::unordered_map<size_t, ResourceEntry> m_hashToEntry;
    std::unordered_map<std::string, size_t> m_pathToHash;

    // Human-readable names (separate from file paths)
    std::unordered_map<size_t, std::string> m_hashToName;
    std::unordered_map<std::string, size_t> m_nameToHash;  // Reverse lookup

    // Submesh -> Material associations (stored during ImportModel)
    // Key: submesh path (e.g., "models/char.mesh#Body")
    // Value: material path (e.g., "materials/body.material")
    std::unordered_map<std::string, std::string> m_submeshToMaterial;

    // Asset -> Source file associations (for grouping by import source)
    // Key: asset path (e.g., "textures/char_diffuse.ktx2")
    // Value: source model path (e.g., "models/char.mesh")
    std::unordered_map<std::string, std::string> m_assetToSource;
};

} // namespace Resource
