// resource/asset_loader.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Resource {
    struct ProcessedTexture;
    struct ProcessedMesh;
    struct ProcessedMaterial;
    struct ProcessedAnimationClip;
    struct ProcessedModel;
    struct MeshFileInfo;
}

namespace Resource {

/**
 * @brief Unified asset loading layer for all file formats
 *
 * AssetLoader provides platform-independent file loading using VFS.
 * All file I/O goes through VFS, ensuring compatibility with:
 * - Desktop: Regular filesystem
 * - Android: APK assets
 * - Future: Archive files (.pak)
 *
 * Supported formats:
 * - .ktx2: Compressed textures (KTX2)
 * - .mesh: Compiled mesh data (custom format)
 * - .material: Material definitions (JSON)
 * - .anim: Animation clips (custom format)
 *
 * Usage:
 *   auto texture = AssetLoader::LoadTexture("textures/foo.ktx2");
 *   auto mesh = AssetLoader::LoadMesh("models/character.mesh");
 */
class AssetLoader
{
public:
    /**
     * @brief Load a KTX2 texture file via VFS
     * @param vfsPath Virtual file system path (e.g., "textures/foo.ktx2")
     * @return ProcessedTexture ready for GPU upload, or empty texture on error
     */
    static ProcessedTexture LoadTexture(const std::string& vfsPath);

    /**
     * @brief Load a compiled mesh file via VFS
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @return ProcessedMesh ready for GPU upload, or empty mesh on error
     */
    static ProcessedMesh LoadMesh(const std::string& vfsPath);

    /**
     * @brief Load a material definition file via VFS
     * @param vfsPath Virtual file system path (e.g., "materials/metal.material")
     * @return ProcessedMaterial ready for GPU upload, or empty material on error
     */
    static ProcessedMaterial LoadMaterial(const std::string& vfsPath);

    /**
     * @brief Load an animation clip file via VFS
     * @param vfsPath Virtual file system path (e.g., "animations/run.anim")
     * @return ProcessedAnimationClip ready for use, or empty clip on error
     */
    static ProcessedAnimationClip LoadAnimation(const std::string& vfsPath);

    /**
     * @brief Load a complete model with all submeshes and material references
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @param materialsBasePath Base path for resolving material references (e.g., "materials/")
     * @return ProcessedModel containing all submeshes, or empty model on error
     */
    static ProcessedModel LoadModel(const std::string& vfsPath, const std::string& materialsBasePath = "");

    /**
     * @brief Query mesh file metadata without loading vertex data
     * Useful for Editor to display submesh information without full load
     * @param vfsPath Virtual file system path (e.g., "models/character.mesh")
     * @return MeshFileInfo with submesh names, material references, counts
     */
    static MeshFileInfo QueryMeshFileInfo(const std::string& vfsPath);

private:
    // Internal parsers (platform-agnostic)
    static ProcessedTexture ParseKTX2(const std::vector<uint8_t>& fileData, const std::string& name);
    static ProcessedMesh ParseMeshFile(const std::vector<uint8_t>& fileData);
    static ProcessedMaterial ParseMaterialFile(const std::vector<uint8_t>& fileData);
    static ProcessedAnimationClip ParseAnimFile(const std::vector<uint8_t>& fileData);
};

} // namespace Resource
