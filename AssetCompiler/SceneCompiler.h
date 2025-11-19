/******************************************************************************/
/*!
\file   SceneCompiler.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Compiles an fbx scene into 3 individual files: 
a .mesh, a .material, and a .ktx2 texture if texture can be found.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include <vector>
#include <filesystem>
#include <string>
#include "CompileOptions.h"
#include "CompilerTypes.h"
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
        std::vector<std::filesystem::path> createdAnimationFiles;

        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };

    class SceneCompiler
    {
    public:
        CompilationResult Compile(const CompilerOptions& compileOptions);

    private:
        SceneProcessingResult ProcessScene(Scene& scene, const MeshOptions& options);

        std::map<TextureDataSource, std::filesystem::path> CompileTextures(const Scene& scene, CompilationResult& result);
        void SaveMeshes(const Scene& scene, CompilationResult& result);
        void SaveMaterialData(const Scene& scene, CompilationResult& result, std::map<TextureDataSource, std::filesystem::path> savedTexturesMap);
        void SaveAnimations(const Scene& scene, CompilationResult& result);
        //void SaveSkeleton(const Scene& scene, CompilationResult& result);

        SceneLoader sceneLoader;
        CompilerOptions options;
    };
}



//---- Below here lies an example of a .material file
/*
{
    "name": "ExampleMaterial",
    "alphaMode": "Opaque",
    "alphaCutoff": 0.5,
    "metallicFactor": 1.0,
    "roughnessFactor": 1.0,
    "normalScale": 1.0,
    "occlusionStrength": 1.0,
    "baseColorFactor": [
        1.0,
        1.0,
        1.0,
        1.0
    ],
    "emissiveFactor": [
        0.0,
        0.0,
        0.0
    ],
    "textures": [
        {
            "key": "baseColor",
            "value": "CompiledAssets\\textures\\colormap.ktx2"
        },
        {
            "key": "emissive",
            "value": ""
        },
        {
            "key": "metallicRoughness",
            "value": ""
        },
        {
            "key": "normal",
            "value": ""
        },
        {
            "key": "occlusion",
            "value": ""
        }
    ],
    "flags": 0
}
*/