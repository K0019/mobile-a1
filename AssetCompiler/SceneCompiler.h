#pragma once
#include <vector>
#include <filesystem>
#include <string>
#include "CompileOptions.h"
#include "SceneLoader.h"

namespace compiler
{
    // Returns and results
    struct SceneProcessingResult
    {
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };

    struct CompilationResult
    {
        bool success = true;
        std::filesystem::path inputPath;

        // Paths to the newly created files.
        std::vector<std::filesystem::path> createdMeshFiles;
        std::vector<std::filesystem::path> createdMaterialFiles;
        std::vector<std::filesystem::path> createdTextureFiles;

        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };



    class SceneCompiler
    {
    public:
        CompilationResult Compile(const CompilerOptions& compileOptions);

    private:
        SceneProcessingResult ProcessScene(Scene& scene, const MeshOptions& options);

        void CompileTextures(const Scene& scene, CompilationResult& result);


        //Save back to disk
        void SaveMeshes(const Scene& scene, CompilationResult& result);
        void SaveMaterialData(const Scene& scene, CompilationResult& result);

        SceneLoader sceneLoader;
        CompilerOptions options;
    };

}