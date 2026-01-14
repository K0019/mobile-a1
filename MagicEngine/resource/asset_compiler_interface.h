// resource/asset_compiler_interface.h
#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace Resource
{

/**
 * @brief Result of an asset compilation operation.
 */
struct CompileResult
{
    bool success = false;

    // Paths to created compiled assets (physical paths)
    std::vector<std::filesystem::path> meshFiles;
    std::vector<std::filesystem::path> textureFiles;
    std::vector<std::filesystem::path> materialFiles;
    std::vector<std::filesystem::path> animationFiles;

    // Error and warning messages
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

/**
 * @brief Interface to the AssetCompiler executable.
 *
 * This class provides methods to compile raw assets (.fbx, .glb, .png, etc.)
 * into engine-ready formats (.mesh, .ktx2, .material, .anim) by invoking
 * the AssetCompiler.exe as a subprocess.
 *
 * Usage:
 *   // Compile a GLB file
 *   CompileResult result = AssetCompilerInterface::CompileAsset("assets/models/character.glb");
 *   if (result.success) {
 *       for (const auto& mesh : result.meshFiles) {
 *           ResourceImporter::Import(mesh);
 *       }
 *   }
 */
class AssetCompilerInterface
{
public:
    /**
     * @brief Set the path to the AssetCompiler executable.
     * @param exePath Path to AssetCompiler.exe
     */
    static void SetCompilerPath(const std::filesystem::path& exePath);

    /**
     * @brief Set the output directory for compiled assets.
     * @param outputDir Directory where compiled assets will be written
     */
    static void SetOutputDirectory(const std::filesystem::path& outputDir);

    /**
     * @brief Set the assets root directory (for relative path calculations).
     * @param assetsRoot Root directory of assets
     */
    static void SetAssetsRoot(const std::filesystem::path& assetsRoot);

    /**
     * @brief Compile a raw asset file and return paths to compiled outputs.
     *
     * @param vfsPath VFS path to the asset (e.g., "assets/models/character.glb")
     * @return CompileResult containing success status and paths to compiled files
     *
     * Supported input formats:
     * - .fbx, .glb, .gltf -> .mesh, .material, .anim
     * - .png, .jpg, .jpeg, .bmp -> .ktx2
     */
    static CompileResult CompileAsset(const std::string& vfsPath);

    /**
     * @brief Compile and immediately import an asset.
     *
     * This is a convenience function that:
     * 1. Compiles the raw asset
     * 2. Imports all resulting compiled assets into ResourceManager
     *
     * @param vfsPath VFS path to the asset
     * @return true if compilation and import succeeded
     */
    static bool CompileAndImport(const std::string& vfsPath);

    /**
     * @brief Check if a file extension is supported for compilation.
     * @param extension File extension including dot (e.g., ".fbx")
     * @return true if the extension can be compiled
     */
    static bool IsCompilableExtension(const std::string& extension);

    /**
     * @brief Get list of extensions that can be compiled.
     */
    static std::vector<std::string> GetCompilableExtensions();

private:
    static std::filesystem::path s_compilerPath;
    static std::filesystem::path s_outputDirectory;
    static std::filesystem::path s_assetsRoot;

    static CompileResult ParseManifest(const std::filesystem::path& manifestPath);
};

} // namespace Resource
