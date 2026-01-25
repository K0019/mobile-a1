/******************************************************************************/
/*!
\file   SceneCompiler.cpp
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

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "SceneCompiler.h"
#include "tools/assets/processing/mesh_process.h"  // Engine's MeshOptimizer
#include "tools/assets/io/mesh_loader.h"           // Engine's calculateBounds
#include "TextureCompiler.h"
#include "ProgressReporter.h"
#include "resource/asset_formats/mesh_format.h"    // Engine's mesh file format
#include "resource/asset_formats/anim_format.h"    // Engine's animation file format
#include "resource/asset_metadata.h"               // For writing .meta sidecar files
#include <string_view>                             // For content-based hashing

// Use engine types
using Resource::MeshOptimizer;
using Resource::MeshFileHeader;
using Resource::MeshNode;
using Resource::MeshInfo;
using Resource::MeshFile_Bone;
using Resource::MeshFile_MorphTarget;
using Resource::MeshFile_MorphDelta;
using Resource::MESH_FILE_MAGIC;
using Resource::AnimationFileHeader;
using Resource::BoneAnimationChannel;
using Resource::AnimationFile_MorphChannel;
using Resource::AnimationFile_MorphKey;
using Resource::ANIM_FILE_MAGIC;
using Resource::MeshLoading::calculateBounds;

#include <fstream>
#include <set>
#include <iostream>
#include <span>
#include <utility>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/ostreamwrapper.h"

namespace compiler
{
    // ----- String helpers ----- //
    static std::string ToLower(const std::string& str)
    {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        return lowerStr;
    }

    static bool EndsWith(const std::string& str, const std::string& suffix)
    {
        if (suffix.length() > str.length()) return false;
        return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }

    // ----- Platform-aware texture format helpers ----- //
    // Get appropriate format for linear dual-channel data (normals, metallic-roughness)
    static TextureCompressionFormat GetLinearTextureFormat(TextureCompressionFormat platformDefault)
    {
        // If platform is Android (ASTC), use ASTC for all textures
        // If platform is Windows (BC7), use BC5 for dual-channel linear data
        if (platformDefault == TextureCompressionFormat::ASTC)
            return TextureCompressionFormat::ASTC;
        return TextureCompressionFormat::BC5;
    }

    // Get appropriate format for single-channel data (occlusion)
    static TextureCompressionFormat GetSingleChannelFormat(TextureCompressionFormat platformDefault)
    {
        if (platformDefault == TextureCompressionFormat::ASTC)
            return TextureCompressionFormat::ASTC;
        return TextureCompressionFormat::BC4;
    }

    // ----- Texture / Filepath helpers ----- //
    struct PathResolutionResult
    {
        std::filesystem::path finalPath; // The correct, true, real, not fake, path that actually exists to a file on the disk.
        std::string warning;
        bool success = false;
    };

    // Try to find the texture file, even if some FELLA gives the wrong filenames
    PathResolutionResult ResolveTexturePath(const std::filesystem::path& sourcePath, const std::filesystem::path& modelBasePath)
    {
        PathResolutionResult result;

        // Perfect match
        if (std::filesystem::exists(sourcePath))
        {
            result.success = true;
            result.finalPath = sourcePath;
            return result;
        }

        // Wrong subfolder case - material says "textures/N00_000_Hair_00_nml.png", but its just "N00_000_Hair_00_nml.png"
        std::filesystem::path textureFileName = sourcePath.filename();
        std::filesystem::path alternatePath = modelBasePath / textureFileName;

        if (std::filesystem::exists(alternatePath))
        {
            result.success = true;
            result.finalPath = alternatePath;
            result.warning =
                "[Path Fix] Texture '" + sourcePath.string() +
                "' was not found. However, a file with the same name was found at '" + alternatePath.string();
            
            return result;
        }

        // Wrong foldername case - material wants "Aji.fbm/_16.png", but its "Aji.vrm1.Textures/_16.png"
        std::filesystem::path searchRoot = modelBasePath;
        try
        {
            if (std::filesystem::exists(searchRoot) && std::filesystem::is_directory(searchRoot))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(searchRoot))
                {
                    // Only check directories
                    if (entry.is_directory())
                    {
                        std::filesystem::path possibleFix = entry.path() / textureFileName;
                        if (std::filesystem::exists(possibleFix))
                        {
                            result.success = true;
                            result.finalPath = possibleFix;
                            result.warning =
                                "[Path Fix] Texture '" + sourcePath.string() +
                                "' was not found. However, a file with the same name was found at '" + possibleFix.string();
                            
                            return result;
                        }
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            result.success = false;
            result.warning = "Filesystem error while searching for '" + sourcePath.string() + "': " + e.what();
            return result;
        }


        // Sorry, im not as good as unity, i cant find the file.
        result.success = false;
        result.warning = "Could not find texture '" + sourcePath.string();
        return result;
    }
    // Parse the filename to determine texture compilation options
    // Uses platform-aware format selection and sets isSRGB based on texture type
    TextureOptions GetTextureOptionsForFile(const std::filesystem::path& resolvedPath, const TextureOptions& defaultOptions)
    {
        TextureOptions newOptions = defaultOptions;
        std::string stem = ToLower(resolvedPath.stem().string());

        // Check for Normal Maps
        // e.g: "N00_000_00_HairBack_00_nml.normal.png", "_11.normal.png"
        if (EndsWith(stem, "_n") || EndsWith(stem, "_nml") || EndsWith(stem, "_normal"))
        {
            newOptions.compressionFormat = GetLinearTextureFormat(defaultOptions.compressionFormat);
            newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
            newOptions.isSRGB = false;  // Normal maps are linear data
            return newOptions;
        }

        // Check for Specular/Roughness/Metallic
        // e.g., "_roughness", "_metallic", "_s", "_r", "_m"
        if (EndsWith(stem, "_s") || EndsWith(stem, "_specular") ||
            EndsWith(stem, "_r") || EndsWith(stem, "_roughness") ||
            EndsWith(stem, "_m") || EndsWith(stem, "_metallic"))
        {
            newOptions.compressionFormat = GetLinearTextureFormat(defaultOptions.compressionFormat);
            newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
            newOptions.isSRGB = false;  // PBR data is linear
            return newOptions;
        }

        // Check for Occlusion/Mask (single channel data)
        if (EndsWith(stem, "_ao") || EndsWith(stem, "_mask") || EndsWith(stem, "_occlusion"))
        {
            newOptions.compressionFormat = GetSingleChannelFormat(defaultOptions.compressionFormat);
            newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
            newOptions.isSRGB = false;  // Occlusion is linear data
            return newOptions;
        }

        // Default: Assume Diffuse/Albedo (color texture)
        // Uses platform default (BC7 for Windows, ASTC for Android)
        newOptions.compressionFormat = defaultOptions.compressionFormat;
        newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
        newOptions.isSRGB = true;  // Color textures need sRGB gamma correction
        return newOptions;
    }


    // ----- Scene Compilation ----- //
    CompilationResult SceneCompiler::Compile(const CompilerOptions& compileOptions)
    {
        options = compileOptions;
        CompilationResult result;
        std::string filename = options.general.inputPath.filename().string();

        ProgressReporter::ReportProgress(0.1f, "Loading scene", filename);
        SceneLoadData loadData = sceneLoader.loadScene(options.general.inputPath, options.mesh);

        if (!loadData.scene)
        {
            result.success = false;
            result.errors = loadData.errors;
            return result;
        }

        Scene scene = std::move(*loadData.scene);

        // Processing stage, meshopt is here
        ProgressReporter::ReportProgress(0.2f, "Processing meshes", filename);
        auto sceneProcessResult = ProcessScene(scene, compileOptions.mesh);

        if (!sceneProcessResult.errors.empty())
        {
            result.errors.insert(result.errors.end(), sceneProcessResult.errors.begin(), sceneProcessResult.errors.end());
            return result;
        }
        if (!sceneProcessResult.warnings.empty())
        {
            result.warnings.insert(result.warnings.end(), sceneProcessResult.warnings.begin(), sceneProcessResult.warnings.end());
        }

        // Save the data
        ProgressReporter::ReportProgress(0.4f, "Saving meshes", filename);
        SaveMeshes(scene, result);

        ProgressReporter::ReportProgress(0.5f, "Compiling textures", filename);
        auto savedTexturesMap = CompileTextures(scene, result);

        ProgressReporter::ReportProgress(0.8f, "Saving materials", filename);
        SaveMaterialData(scene, result, savedTexturesMap);

        ProgressReporter::ReportProgress(0.9f, "Saving animations", filename);
        SaveAnimations(scene, result);

        if (!result.errors.empty())
        {
            result.success = false;
            std::cout << "\n----------------- ERROR WHEN COMPILING SCENE -----------------\n";
            for (const auto& error : result.errors)
            {
                std::cout << "ERROR: " << error << "\n";
            }
        }

        for (const auto& warning : result.warnings)
        {
            std::cout << "Warning: " << warning << "\n";
        }

        return result;
    }

    SceneProcessingResult SceneCompiler::ProcessScene(Scene& scene, const MeshOptions& inOptions)
    {
        SceneProcessingResult processingResult;
        for (ProcessedMesh& mesh : scene.meshes)
        {
            if (mesh.vertices.empty() || mesh.indices.empty())
            {
                continue;
            }

            if (inOptions.generateTangents)
            {
                if (!MeshOptimizer::generateTangents(mesh.vertices, mesh.indices, 
                                                     mesh.skinning.empty() ? nullptr : &mesh.skinning, mesh.morphTargets.empty() ? nullptr : &mesh.morphTargets))
                {
                    processingResult.warnings.push_back("MeshOptimizer::generateTangents failed");
                }
            }

            if (inOptions.optimize && MeshOptimizer::shouldOptimize(mesh.vertices, mesh.indices))
            {
                auto result = MeshOptimizer::optimize(mesh.vertices, mesh.indices, 
                                                      mesh.skinning.empty() ? nullptr : &mesh.skinning, mesh.morphTargets.empty() ? nullptr : &mesh.morphTargets);

                if (!result.success)
                {
                    processingResult.warnings.push_back("MeshOptimizer::optimize failed");
                }
            }

            if (inOptions.calculateBounds)
            {
                mesh.bounds = calculateBounds(mesh.vertices, mesh.morphTargets.empty() ? nullptr : &mesh.morphTargets);
            }
        }
        return processingResult;
    }


    // ----- Saving to disk ----- //
    std::map<TextureDataSource, std::filesystem::path> SceneCompiler::CompileTextures(const Scene& scene, CompilationResult& result)
    {
        std::map<TextureDataSource, std::filesystem::path> compiledTextures;
        std::set<std::pair<std::string, TextureDataSource>> uniqueTextureSources;
        
        for (const auto& material : scene.materials)
        {
            for (const auto& [type, source] : material.texturePaths)
            {
                if (source.index() != 0) // Not std::monostate
                {
                    uniqueTextureSources.insert(std::make_pair(type, source));
                }
            }
        }

        if (uniqueTextureSources.empty())
        {
            return compiledTextures;
        }

        // We just assume that everything inside textureSources are filepaths if the first element is one, because why would it be anything else
        if (std::holds_alternative<FilePathSource>(std::get<TextureDataSource>(*uniqueTextureSources.begin())))
        {
            // Resolve paths, make sure they actually exists
            std::vector<std::string> texturePathErrors;
            std::filesystem::path modelBasePath = options.general.inputPath.parent_path();
            std::map<std::filesystem::path, std::filesystem::path> resolvedTexturePaths;

            for (auto& [key, source] : uniqueTextureSources)
            {
                std::filesystem::path sourcePath = std::get<FilePathSource>(source).path;
                PathResolutionResult pathResolution = ResolveTexturePath(sourcePath, modelBasePath);

                if (pathResolution.success)
                {
                    if (!pathResolution.warning.empty())
                    {
                        result.warnings.push_back(pathResolution.warning);
                    }
                    // This path is confirmed to exist on disk, safe to send to texturecompiler
                    resolvedTexturePaths[sourcePath] = pathResolution.finalPath;
                }
                else
                {
                    // Cannot find the file anywhere.
                    texturePathErrors.push_back(pathResolution.warning);
                }
            }

            if (!texturePathErrors.empty())
            {
                result.errors.push_back("Texture path errors: Could not find one or more textures. Texture compilation aborted entirely.");
                result.errors.insert(result.errors.end(), texturePathErrors.begin(), texturePathErrors.end());
                return compiledTextures;
            }

            compiler::TextureCompiler texCompiler;

            for (const auto& [originalPath, resolvedPath] : resolvedTexturePaths)
            {
                CompilerOptions texOpts = options;
                texOpts.general.inputPath = resolvedPath; // Use the *correct* path
                //texOpts.general.outputPath = textureOutputDir;
                texOpts.texture = options.texture;

                // Parse filename to set specific options and isSRGB based on texture type
                // (e.g., normal maps use BC5/ASTC with linear, color textures use BC7/ASTC with sRGB)
                texOpts.texture = GetTextureOptionsForFile(resolvedPath, options.texture);

                auto texCompileResult = texCompiler.Compile(texOpts);
                if (texCompileResult.errors.empty())
                {
                    std::string outputFilename = originalPath.stem().string() + ".ktx2";
                    //result.createdTextureFiles.push_back(options.general.outputPath / "textures" / outputFilename);
                    result.createdTextureFiles.push_back(texCompileResult.createdTextureFiles[0]);
                    FilePathSource compiledFilePathSource {originalPath.string()};
                    compiledTextures[compiledFilePathSource] = texCompileResult.createdTextureFiles[0];
                }
                else
                {
                    result.errors.push_back("Failed to compile texture: " + resolvedPath.string());
                }
            }
        }
        else // Assume its an embedded texture...
        {
            compiler::TextureCompiler texCompiler;

            for (const auto& [key, source] : uniqueTextureSources)
            {
                CompilerOptions texOpts = options;

                EmbeddedTextureSource embdeddedSource = std::get<EmbeddedTextureSource>(source);

                std::string textureFilename;

                // Fix internal texture name (the name inside the glb file)
                if (!embdeddedSource.name.empty())
                {
                    textureFilename = std::filesystem::path(embdeddedSource.name).stem().string();
                }
                else
                {
                    // Create a hash based name from the actual texture data content
                    const uint8_t* dataPtr = embdeddedSource.compressedData ? embdeddedSource.compressedData : embdeddedSource.rawData;
                    // Raw data size is width * height * 4 (RGBA)
                    size_t dataSize = embdeddedSource.compressedData ? embdeddedSource.compressedSize
                        : (static_cast<size_t>(embdeddedSource.width) * embdeddedSource.height * 4);

                    // Hash the actual data content, not the pointer address
                    std::string_view dataView(reinterpret_cast<const char*>(dataPtr), dataSize);
                    size_t contentHash = std::hash<std::string_view>{}(dataView);

                    textureFilename = options.general.inputPath.stem().string() + std::to_string(contentHash);
                    embdeddedSource.name = textureFilename;
                }

                // Setup texture options based on texture type
                // Uses platform-aware format selection (BC7/BC5/BC4 for Windows, ASTC for Android)
                if (key == texturekeys::BASE_COLOR || key == texturekeys::EMISSIVE)
                {
                    // Color textures: use platform format (BC7/ASTC), enable sRGB
                    texOpts.texture.compressionFormat = options.texture.compressionFormat;
                    texOpts.texture.channelFormat = TextureChannelFormat::RGBA_8888;
                    texOpts.texture.isSRGB = true;  // Color data needs gamma correction
                }
                else if (key == texturekeys::NORMAL)
                {
                    // Normal maps: BC5 on desktop, ASTC on Android (linear)
                    texOpts.texture.compressionFormat = GetLinearTextureFormat(options.texture.compressionFormat);
                    texOpts.texture.channelFormat = TextureChannelFormat::RGBA_8888;
                    texOpts.texture.isSRGB = false;  // Normal maps are linear data
                }
                else if (key == texturekeys::METALLIC_ROUGHNESS)
                {
                    // Packed PBR data: BC5 on desktop, ASTC on Android (linear)
                    texOpts.texture.compressionFormat = GetLinearTextureFormat(options.texture.compressionFormat);
                    texOpts.texture.channelFormat = TextureChannelFormat::RGBA_8888;
                    texOpts.texture.isSRGB = false;  // PBR data is linear
                }
                else if (key == texturekeys::OCCLUSION)
                {
                    // Single channel data: BC4 on desktop, ASTC on Android (linear)
                    texOpts.texture.compressionFormat = GetSingleChannelFormat(options.texture.compressionFormat);
                    texOpts.texture.channelFormat = TextureChannelFormat::RGBA_8888;
                    texOpts.texture.isSRGB = false;  // Occlusion is linear data
                }

                // Fix output filepath
                std::filesystem::path relativeDir = std::filesystem::relative(options.general.inputPath.parent_path(), options.general.assetsRoot);
                std::filesystem::path assetContainerName = options.general.inputPath.stem();
                std::filesystem::path assetOutputDir = options.general.outputPath / relativeDir / assetContainerName;
                texOpts.general.inputPath = "";
                texOpts.general.outputPath = assetOutputDir;

                auto texCompileResult = texCompiler.CompileFromMemory(embdeddedSource, texOpts);

                if (result.errors.empty())
                {
                    std::string outputFilename = textureFilename + ".ktx2";
                    result.createdTextureFiles.push_back(texCompileResult.createdTextureFiles[0]);
                    compiledTextures[source] = texCompileResult.createdTextureFiles[0];
                }
            }
        }

        return compiledTextures;
    }

    void SceneCompiler::SaveMeshes(const Scene& scene, CompilationResult& result)
    {
        std::vector<MeshNode> finalNodes;
		std::vector<MeshInfo> finalMeshInfos;
		std::string meshNames; // String with names separated by \0s
		std::string materialNames; // String with names separated by \0s
		std::vector<uint32_t> finalIndices;
		std::vector<Vertex> finalVertices;
        
        std::vector<SkinningData> finalSkinningData;
        std::vector<MeshFile_MorphTarget> finalMorphTargets;
        std::vector<MeshFile_MorphDelta>  finalMorphDeltas;
        std::string morphTargetNameBuffer;

		// Put material names into lookup map
		std::map<uint32_t, uint32_t> materialIndexToNameOffset;
		uint32_t currentNameOffset = 0;
		for (uint32_t i = 0; i < scene.materials.size(); ++i)
		{
			materialIndexToNameOffset[i] = currentNameOffset;
			// Use lowercase material names for consistent lookup at runtime
			materialNames += ToLower(scene.materials[i].name);
			materialNames += '\0';
			currentNameOffset = static_cast<uint32_t>(materialNames.size());
		}
		
		//Set up Mesh vertex and index buffers
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (const auto& mesh : scene.meshes)
		{
			MeshInfo meshInfo;
			meshInfo.firstIndex = indexOffset;
			meshInfo.indexCount = static_cast<uint32_t>(mesh.indices.size());
			meshInfo.firstVertex = vertexOffset;
			meshInfo.materialNameIndex = materialIndexToNameOffset[mesh.materialIndex];
			meshInfo.meshBounds = mesh.bounds;

            meshInfo.nameOffset = static_cast<uint32_t>(meshNames.size());
            meshNames += mesh.name; // This is the aiMesh->mName
            meshNames += '\0';

            meshInfo.firstMorphTarget = static_cast<uint32_t>(finalMorphTargets.size());
            meshInfo.morphTargetCount = static_cast<uint32_t>(mesh.morphTargets.size());

			finalMeshInfos.push_back(meshInfo);

			finalVertices.insert(finalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            finalSkinningData.insert(finalSkinningData.end(), mesh.skinning.begin(), mesh.skinning.end());

			for (uint32_t index : mesh.indices)
			{
				finalIndices.push_back(index + vertexOffset);
			}

            for (const auto& processedTarget : mesh.morphTargets)
            {
                MeshFile_MorphTarget target;
                target.nameOffset = static_cast<uint32_t>(morphTargetNameBuffer.size());
                morphTargetNameBuffer += processedTarget.name;
                morphTargetNameBuffer += '\0';

                target.firstDelta = static_cast<uint32_t>(finalMorphDeltas.size());
                target.deltaCount = static_cast<uint32_t>(processedTarget.deltas.size());

                // Copy the deltas
                for (const auto& processedDelta : processedTarget.deltas)
                {
                    // We must add the vertexOffset to the delta's index
                    // so it correctly points into the final global vertex buffer
                    finalMorphDeltas.push_back({
                        processedDelta.vertexIndex + vertexOffset,
                        processedDelta.deltaPosition,
                        processedDelta.deltaNormal,
                        processedDelta.deltaTangent
                        });
                }
                finalMorphTargets.push_back(target);
            }

			vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
			indexOffset += static_cast<uint32_t>(mesh.indices.size());
		}

		for (const auto& compilerNode : scene.nodes)
		{
			MeshNode node;
			node.transform = compilerNode.localTransform;
			node.meshIndex = compilerNode.meshIndex;
			node.parentIndex = compilerNode.parentIndex;

			strncpy_s(node.name, sizeof(node.name), compilerNode.name.c_str(), sizeof(node.name) - 1);
			node.name[sizeof(node.name) - 1] = '\0';

			finalNodes.push_back(node);
		}

        // Prepare skelly data
        std::vector<MeshFile_Bone> finalBones;
        std::string boneNameBuffer;

        if (!scene.skeleton.bones.empty())
        {
            finalBones.reserve(scene.skeleton.bones.size());
            uint32_t currentBoneNameOffset = 0;

            for (const auto& processedBone : scene.skeleton.bones)
            {
                MeshFile_Bone bone;
                bone.inverseBindPose = processedBone.inverseBindPose;
                bone.bindPose = processedBone.bindPose;
                bone.parentIndex = processedBone.parentIndex;

                bone.nameOffset = currentBoneNameOffset;
                boneNameBuffer += processedBone.name;
                boneNameBuffer += '\0';
                currentBoneNameOffset = (uint32_t)boneNameBuffer.size();

                finalBones.push_back(bone);
            }
        }


		// Bounds calculation would be placed here.
        // Completely unused right now, but may be a performance improvement in the future

		// Populate file header
		MeshFileHeader header;
		header.magic = MESH_FILE_MAGIC;
		header.numNodes = static_cast<uint32_t>(finalNodes.size());
		header.numMeshes = static_cast<uint32_t>(finalMeshInfos.size());
		header.totalIndices = static_cast<uint32_t>(finalIndices.size());
		header.totalVertices = static_cast<uint32_t>(finalVertices.size());
        header.meshNameBufferSize = static_cast<uint32_t>(meshNames.size());
		header.materialNameBufferSize = static_cast<uint32_t>(materialNames.size());

        header.hasSkeleton = finalBones.empty() ? 0 : 1;
        header.numBones = (uint32_t)finalBones.size();
        header.boneNameBufferSize = (uint32_t)boneNameBuffer.size();

        header.hasMorphs = finalMorphTargets.empty() ? 0 : 1;
        header.numMorphTargets = (uint32_t)finalMorphTargets.size();
        header.numMorphDeltas = (uint32_t)finalMorphDeltas.size();
        header.morphTargetNameBufferSize = (uint32_t)morphTargetNameBuffer.size();

		header.sceneBoundsCenter = scene.center;
		header.sceneBoundsRadius = scene.radius;
		header.sceneBoundsMin = scene.boundingMin;
		header.sceneBoundsMax = scene.boundingMax;


		// Calculate data offsets
		uint64_t currentOffset = sizeof(MeshFileHeader);

        header.nodeDataOffset = currentOffset;
        currentOffset += finalNodes.size() * sizeof(MeshNode);

        header.meshInfoDataOffset = currentOffset;
        currentOffset += finalMeshInfos.size() * sizeof(MeshInfo);

        header.meshNamesOffset = currentOffset;
        currentOffset += meshNames.size();

        header.materialNamesOffset = currentOffset;
        currentOffset += materialNames.size();

        header.indexDataOffset = currentOffset;
        currentOffset += finalIndices.size() * sizeof(uint32_t);

        header.vertexDataOffset = currentOffset;
        currentOffset += finalVertices.size() * sizeof(Vertex);

        header.skinningDataOffset = currentOffset;
        currentOffset += finalSkinningData.size() * sizeof(SkinningData);

        header.boneDataOffset = currentOffset;
        currentOffset += finalBones.size() * sizeof(MeshFile_Bone);

        header.boneNameOffset = currentOffset;
        currentOffset += boneNameBuffer.size();

        header.morphTargetDataOffset = currentOffset;
        currentOffset += finalMorphTargets.size() * sizeof(MeshFile_MorphTarget);

        header.morphDeltaDataOffset = currentOffset;
        currentOffset += finalMorphDeltas.size() * sizeof(MeshFile_MorphDelta);

        header.morphTargetNameOffset = currentOffset;
        currentOffset += morphTargetNameBuffer.size();


		// Writing to file timeu~~ :3
        std::filesystem::path relativeDir = std::filesystem::relative(options.general.inputPath.parent_path(), options.general.assetsRoot);
        std::filesystem::path assetContainerName = options.general.inputPath.stem();
        std::filesystem::path assetOutputDir = options.general.outputPath / relativeDir / assetContainerName;
        std::filesystem::create_directories(assetOutputDir);
        
		std::filesystem::path outFilePath = assetOutputDir / (options.general.inputPath.stem().string() + ".mesh");
		std::ofstream outFile(outFilePath, std::ios::binary);

        // Mesh data
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(MeshFileHeader));
        outFile.write(reinterpret_cast<const char*>(finalNodes.data()), finalNodes.size() * sizeof(MeshNode));
        outFile.write(reinterpret_cast<const char*>(finalMeshInfos.data()), finalMeshInfos.size() * sizeof(MeshInfo));
        outFile.write(meshNames.c_str(), meshNames.size());
        outFile.write(materialNames.c_str(), materialNames.size());
        outFile.write(reinterpret_cast<const char*>(finalIndices.data()), finalIndices.size() * sizeof(uint32_t));
        outFile.write(reinterpret_cast<const char*>(finalVertices.data()), finalVertices.size() * sizeof(Vertex));
        //Skellyton data
        outFile.write(reinterpret_cast<const char*>(finalSkinningData.data()), finalSkinningData.size() * sizeof(SkinningData));
        outFile.write(reinterpret_cast<const char*>(finalBones.data()), finalBones.size() * sizeof(MeshFile_Bone));
        outFile.write(boneNameBuffer.c_str(), boneNameBuffer.size());
        // Morph data blok
        outFile.write(reinterpret_cast<const char*>(finalMorphTargets.data()), finalMorphTargets.size() * sizeof(MeshFile_MorphTarget));
        outFile.write(reinterpret_cast<const char*>(finalMorphDeltas.data()), finalMorphDeltas.size() * sizeof(MeshFile_MorphDelta));
        outFile.write(morphTargetNameBuffer.c_str(), morphTargetNameBuffer.size());

		outFile.close();

        result.createdMeshFiles.push_back(outFilePath);

        // Write .meta sidecar file with source tracking
        Resource::AssetMetadata meshMeta;
        meshMeta.assetType = AssetFormat::AssetType::Mesh;
        meshMeta.sourcePath = GetRelativeSourcePath(options.general.inputPath, options.general.assetsRoot);
        meshMeta.platform = GetPlatformName(options.general.platform);
        meshMeta.sourceTimestamp = static_cast<uint64_t>(
            std::filesystem::last_write_time(options.general.inputPath).time_since_epoch().count());
        meshMeta.compiledTimestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count());
        meshMeta.formatVersion = MESH_FILE_MAGIC;  // Use magic as version identifier
        meshMeta.saveToFile(Resource::AssetMetadata::getMetaPath(outFilePath));
    }

    void SceneCompiler::SaveMaterialData(const Scene& scene, CompilationResult& result, std::map<TextureDataSource, std::filesystem::path> savedTexturesMap)
    {
        for (const ProcessedMaterialSlot& materialSlot : scene.materials)
        {
            rapidjson::Document doc;
            auto& allocator = doc.GetAllocator();
            doc.SetObject();

            doc.AddMember("name", rapidjson::Value(materialSlot.name.c_str(), allocator), allocator);

            std::string alphaModeStr;
            switch (materialSlot.alphaMode)
            {
            case AlphaMode::Opaque:
                alphaModeStr = "Opaque";
                break;
            case AlphaMode::Mask:
                alphaModeStr = "Mask";
                break;
            case AlphaMode::Blend:
                alphaModeStr = "Blend";
                break;
            default:
                alphaModeStr = "Opaque";
            }
            doc.AddMember("alphaMode", rapidjson::Value(alphaModeStr.c_str(), allocator), allocator);

            doc.AddMember("alphaCutoff", materialSlot.alphaCutoff, allocator);
            doc.AddMember("metallicFactor", materialSlot.metallicFactor, allocator);
            doc.AddMember("roughnessFactor", materialSlot.roughnessFactor, allocator);
            doc.AddMember("normalScale", materialSlot.normalScale, allocator);
            doc.AddMember("occlusionStrength", materialSlot.occlusionStrength, allocator);

            rapidjson::Value baseColor(rapidjson::kArrayType);
            baseColor.PushBack(materialSlot.baseColorFactor.x, allocator);
            baseColor.PushBack(materialSlot.baseColorFactor.y, allocator);
            baseColor.PushBack(materialSlot.baseColorFactor.z, allocator);
            baseColor.PushBack(materialSlot.baseColorFactor.w, allocator);
            doc.AddMember("baseColorFactor", baseColor, allocator);

            rapidjson::Value emissive(rapidjson::kArrayType);
            emissive.PushBack(materialSlot.emissiveFactor.x, allocator);
            emissive.PushBack(materialSlot.emissiveFactor.y, allocator);
            emissive.PushBack(materialSlot.emissiveFactor.z, allocator);
            doc.AddMember("emissiveFactor", emissive, allocator);


            rapidjson::Value texturesArray(rapidjson::kArrayType);

            for (const char* key : texturekeys::ALL)
            {
                rapidjson::Value textureEntry(rapidjson::kObjectType);
                textureEntry.AddMember("key", rapidjson::Value(key, allocator), allocator);

                std::string valueStr = "";
                auto it = materialSlot.texturePaths.find(key);
                if (it != materialSlot.texturePaths.end())
                {
                    auto mapIt = savedTexturesMap.find(it->second);
                    if (mapIt != savedTexturesMap.end())
                    {
                        std::filesystem::path relativeDir = std::filesystem::relative(mapIt->second, options.general.assetsRoot);
                        valueStr = relativeDir.generic_string(); // Use forward slashes for cross-platform compatibility
                    }
                }

                textureEntry.AddMember("value", rapidjson::Value(valueStr.c_str(), allocator), allocator);
                texturesArray.PushBack(textureEntry, allocator);
            }
            doc.AddMember("textures", texturesArray, allocator);

            doc.AddMember("flags", materialSlot.flags, allocator);

            // Write to disk
            std::filesystem::path relativeDir = std::filesystem::relative(options.general.inputPath.parent_path(), options.general.assetsRoot);
            std::filesystem::path assetContainerName = options.general.inputPath.stem();
            std::filesystem::path assetOutputDir = options.general.outputPath / relativeDir / assetContainerName;
            std::filesystem::create_directories(assetOutputDir);

            // Use lowercase filename for consistent lookup at runtime
            std::filesystem::path outFilePath = assetOutputDir / (ToLower(materialSlot.name) + ".material");

            std::ofstream outFile(outFilePath);
            rapidjson::OStreamWrapper osw(outFile);

            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);

            outFile.close();

            result.createdMaterialFiles.push_back(outFilePath);

            // Write .meta sidecar file with source tracking
            Resource::AssetMetadata matMeta;
            matMeta.assetType = AssetFormat::AssetType::Material;
            matMeta.sourcePath = GetRelativeSourcePath(options.general.inputPath, options.general.assetsRoot);
            matMeta.platform = GetPlatformName(options.general.platform);
            matMeta.sourceTimestamp = static_cast<uint64_t>(
                std::filesystem::last_write_time(options.general.inputPath).time_since_epoch().count());
            matMeta.compiledTimestamp = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count());
            matMeta.formatVersion = 1;  // Material format version
            matMeta.saveToFile(Resource::AssetMetadata::getMetaPath(outFilePath));
        }
    }

    void SceneCompiler::SaveAnimations(const Scene& scene, CompilationResult& result)
    {
        if (scene.animations.empty())
        {
            return; // No animations to save
        }

        std::filesystem::path relativeDir = std::filesystem::relative(options.general.inputPath.parent_path(), options.general.assetsRoot);
        std::filesystem::path assetContainerName = options.general.inputPath.stem();
        std::filesystem::path assetOutputDir = options.general.outputPath / relativeDir / assetContainerName;
        std::filesystem::create_directories(assetOutputDir);

        std::string stem = options.general.inputPath.stem().string();

        // This is needed to resolve the meshName in ProcessedMorphChannel
        std::map<std::string, uint32_t> meshNameToIndex;
        for (uint32_t i = 0; i < scene.meshes.size(); ++i)
        {
            meshNameToIndex[scene.meshes[i].name] = i;
        }
        for (const auto& node : scene.nodes)    //Alternate resolving method
        {
            if (node.meshIndex != -1)
            {
                meshNameToIndex[node.name] = (uint32_t)node.meshIndex;
            }
        }

        // Loop and save one file PER animation clip
        for (const auto& anim : scene.animations)
        {
            ProcessedAnimation animToSave = anim;

            // --- Buffers for Skeletal Data ---
            std::vector<BoneAnimationChannel> finalSkeletalChannels;
            std::vector<char> skeletalKeyframeBuffer;
            std::string skeletalNameBuffer;

            // --- Buffers for Morph Data ---
            std::vector<AnimationFile_MorphChannel> finalMorphChannels;
            std::vector<AnimationFile_MorphKey> finalMorphKeys;
            std::vector<uint32_t> morphIndexBuffer;
            std::vector<float> morphWeightBuffer;


            // --- Process Skeletal Channels ---
            for (const auto& channel : animToSave.skeletalChannels) //
            {
                if (channel.positionKeys.empty() && channel.rotationKeys.empty() && channel.scaleKeys.empty())
                {
                    continue; // Skip bones that arent animated
                }

                BoneAnimationChannel finalChannel; 
                finalChannel.nameOffset = static_cast<uint32_t>(skeletalNameBuffer.size()); 
                skeletalNameBuffer += channel.nodeName;
                skeletalNameBuffer += '\0';

                // Position keys
                finalChannel.numPositionKeys = (uint32_t)channel.positionKeys.size();
                finalChannel.positionKeyOffset = skeletalKeyframeBuffer.size();
                skeletalKeyframeBuffer.insert(skeletalKeyframeBuffer.end(),
                    reinterpret_cast<const char*>(channel.positionKeys.data()),
                    reinterpret_cast<const char*>(channel.positionKeys.data() + channel.positionKeys.size()));

                // Rotation keys
                finalChannel.numRotationKeys = (uint32_t)channel.rotationKeys.size();
                finalChannel.rotationKeyOffset = skeletalKeyframeBuffer.size();
                skeletalKeyframeBuffer.insert(skeletalKeyframeBuffer.end(),
                    reinterpret_cast<const char*>(channel.rotationKeys.data()),
                    reinterpret_cast<const char*>(channel.rotationKeys.data() + channel.rotationKeys.size()));

                // Scale keys
                finalChannel.numScaleKeys = (uint32_t)channel.scaleKeys.size();
                finalChannel.scaleKeyOffset = skeletalKeyframeBuffer.size();
                skeletalKeyframeBuffer.insert(skeletalKeyframeBuffer.end(),
                    reinterpret_cast<const char*>(channel.scaleKeys.data()),
                    reinterpret_cast<const char*>(channel.scaleKeys.data() + channel.scaleKeys.size()));

                finalSkeletalChannels.push_back(finalChannel);
            }

            // --- Process Morph Channels ---
            for (const auto& channel : animToSave.morphChannels) //
            {
                if (channel.keys.empty())
                {
                    continue; // Skip meshes that aren't animated
                }

                AnimationFile_MorphChannel finalChannel;

                // Resolve mesh name to index
                auto it = meshNameToIndex.find(channel.meshName);
                if (it == meshNameToIndex.end())
                {
                    // This mesh isn't in our scene, skip this track
                    continue;
                }
                finalChannel.meshIndex = it->second;
                finalChannel.numKeys = (uint32_t)channel.keys.size();
                finalChannel.keyOffset = finalMorphKeys.size() * sizeof(AnimationFile_MorphKey);

                // Process all keys in this channel
                for (const auto& key : channel.keys)
                {
                    AnimationFile_MorphKey finalKey;
                    finalKey.time = key.time;
                    finalKey.numTargets = (uint32_t)key.targetIndices.size();

                    // Get offsets into the final data blobs
                    finalKey.targetIndexOffset = morphIndexBuffer.size() * sizeof(uint32_t);
                    finalKey.weightOffset = morphWeightBuffer.size() * sizeof(float);

                    // Append this key's data to the blobs
                    morphIndexBuffer.insert(morphIndexBuffer.end(), key.targetIndices.begin(), key.targetIndices.end());
                    morphWeightBuffer.insert(morphWeightBuffer.end(), key.weights.begin(), key.weights.end());

                    finalMorphKeys.push_back(finalKey);
                }

                finalMorphChannels.push_back(finalChannel);
            }

            if (finalSkeletalChannels.empty() && finalMorphChannels.empty())
            {
                continue; // No data to save for this clip
            }

            // --- Populate File Header ---
            AnimationFileHeader header; //
            header.magic = ANIM_FILE_MAGIC;
            header.duration = animToSave.duration;
            header.ticksPerSecond = animToSave.ticksPerSecond;

            uint64_t currentOffset = sizeof(AnimationFileHeader);

            // Skeletal offsets
            header.numChannels = static_cast<uint32_t>(finalSkeletalChannels.size());
            header.channelDataOffset = currentOffset;
            currentOffset += finalSkeletalChannels.size() * sizeof(BoneAnimationChannel);

            header.keyframeDataOffset = currentOffset;
            currentOffset += skeletalKeyframeBuffer.size();

            header.skeletalNameBufferSize = static_cast<uint32_t>(skeletalNameBuffer.size());
            header.skeletalNameBufferOffset = currentOffset;
            currentOffset += skeletalNameBuffer.size();

            // Morph offsets
            header.numMorphChannels = static_cast<uint32_t>(finalMorphChannels.size());
            header.morphChannelDataOffset = currentOffset;
            currentOffset += finalMorphChannels.size() * sizeof(AnimationFile_MorphChannel);

            header.morphKeyDataOffset = currentOffset;
            currentOffset += finalMorphKeys.size() * sizeof(AnimationFile_MorphKey);

            header.morphIndexDataOffset = currentOffset;
            currentOffset += morphIndexBuffer.size() * sizeof(uint32_t);

            header.morphWeightDataBufferSize = static_cast<uint32_t>(morphWeightBuffer.size() * sizeof(float));
            header.morphWeightDataOffset = currentOffset;
            currentOffset += morphWeightBuffer.size() * sizeof(float);


            // --- Write to Disk ---
            std::string animFilename = stem + animToSave.name + ".anim";
            std::filesystem::path outFilePath = assetOutputDir / animFilename;

            std::ofstream outFile(outFilePath, std::ios::binary);
            if (!outFile.is_open())
            {
                result.errors.push_back("Failed to create animation file: " + outFilePath.string());
                continue;
            }

            outFile.write(reinterpret_cast<const char*>(&header), sizeof(AnimationFileHeader));

            // Skeletal data
            outFile.write(reinterpret_cast<const char*>(finalSkeletalChannels.data()), finalSkeletalChannels.size() * sizeof(BoneAnimationChannel));
            outFile.write(skeletalKeyframeBuffer.data(), skeletalKeyframeBuffer.size()); 
            outFile.write(skeletalNameBuffer.c_str(), skeletalNameBuffer.size());

            // Morph data
            outFile.write(reinterpret_cast<const char*>(finalMorphChannels.data()), finalMorphChannels.size() * sizeof(AnimationFile_MorphChannel));
            outFile.write(reinterpret_cast<const char*>(finalMorphKeys.data()), finalMorphKeys.size() * sizeof(AnimationFile_MorphKey));
            outFile.write(reinterpret_cast<const char*>(morphIndexBuffer.data()), morphIndexBuffer.size() * sizeof(uint32_t));
            outFile.write(reinterpret_cast<const char*>(morphWeightBuffer.data()), morphWeightBuffer.size() * sizeof(float));

            outFile.close();

            result.createdAnimationFiles.push_back(outFilePath);

            // Write .meta sidecar file with source tracking
            Resource::AssetMetadata animMeta;
            animMeta.assetType = AssetFormat::AssetType::Animation;
            animMeta.sourcePath = GetRelativeSourcePath(options.general.inputPath, options.general.assetsRoot);
            animMeta.platform = GetPlatformName(options.general.platform);
            animMeta.sourceTimestamp = static_cast<uint64_t>(
                std::filesystem::last_write_time(options.general.inputPath).time_since_epoch().count());
            animMeta.compiledTimestamp = static_cast<uint64_t>(
                std::chrono::system_clock::now().time_since_epoch().count());
            animMeta.formatVersion = ANIM_FILE_MAGIC;  // Use magic as version identifier
            animMeta.saveToFile(Resource::AssetMetadata::getMetaPath(outFilePath));
        }
    }
}