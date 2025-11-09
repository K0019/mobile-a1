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

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "SceneCompiler.h"
#include "MeshProcessor.h"
#include "TextureCompiler.h"
#include "MeshFileStructure.h"

#include <fstream>
#include <set>
#include <iostream>
#include <span>

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
    TextureOptions GetTextureOptionsForFile(const std::filesystem::path& resolvedPath, const TextureOptions& defaultOptions)
    {
        TextureOptions newOptions = defaultOptions;
        std::string stem = ToLower(resolvedPath.stem().string());

        // Check for Normal Maps
        // e.g: "N00_000_00_HairBack_00_nml.normal.png", "_11.normal.png"
        if (EndsWith(stem, "_n") || EndsWith(stem, "_nml") || EndsWith(stem, "_normal"))
        {
            newOptions.compressionFormat = TextureCompressionFormat::BC5;
            newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
            return newOptions;
        }

        // Check for Specular/Roughness/Metallic/Mask
        // e.g., "_roughness", "_metallic", "_s", "_r", "_m", "_mask"
        if (EndsWith(stem, "_s") || EndsWith(stem, "_specular") ||
            EndsWith(stem, "_r") || EndsWith(stem, "_roughness") ||
            EndsWith(stem, "_m") || EndsWith(stem, "_metallic") ||
            EndsWith(stem, "_ao") || EndsWith(stem, "_mask"))
        {
            newOptions.compressionFormat = TextureCompressionFormat::BC4;
            newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
            return newOptions;
        }

        // Default: Assume Diffuse/Albedo
        newOptions.compressionFormat = TextureCompressionFormat::BC7;
        newOptions.channelFormat = TextureChannelFormat::RGBA_8888;
        return newOptions;
    }


    // ----- Scene Compilation ----- //
    CompilationResult SceneCompiler::Compile(const CompilerOptions& compileOptions)
    {
        options = compileOptions;
        CompilationResult result;
        result.inputPath = options.general.inputPath;

        SceneLoadData loadData = sceneLoader.loadScene(options.general.inputPath, options.mesh);

        if (!loadData.scene)
        {
            result.success = false;
            result.errors = loadData.errors;
            return result;
        }

        Scene scene = std::move(*loadData.scene);
        
        // Processing stage, meshopt is here
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
        SaveMeshes(scene, result);
        SaveMaterialData(scene, result);
        CompileTextures(scene, result);

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

            if (inOptions.optimize && MeshOptimizer::shouldOptimize(mesh.vertices, mesh.indices))
            {
                auto result = MeshOptimizer::optimize(mesh.vertices, mesh.indices, inOptions.generateTangents);

                if (!result.success)
                {
                    processingResult.warnings.push_back("MeshOptimizer::optimize failed");
                }
            }

            if (inOptions.generateTangents)
            {
                if (!MeshOptimizer::generateTangents(mesh.vertices, mesh.indices))
                {
                    processingResult.warnings.push_back("MeshOptimizer::generateTangents failed");
                }
            }

            if (inOptions.calculateBounds)
            {
                mesh.bounds = MeshOptimizer::calculateBounds(mesh.vertices);
            }
        }
        return processingResult;
    }


    // ----- Saving to disk ----- //
    void SceneCompiler::CompileTextures(const Scene& scene, CompilationResult& result)
    {
        std::set<std::filesystem::path> uniqueTexturePaths;
        for (const auto& material : scene.materials)
        {
            for (const auto& [type, path] : material.texturePaths)
            {
                if (!path.empty())
                {
                    uniqueTexturePaths.insert(path);
                }
            }
        }

        if (uniqueTexturePaths.empty())
        {
            return;
        }

        // Resolve paths, make sure they actually exists
        std::vector<std::string> texturePathErrors;
        std::filesystem::path modelBasePath = options.general.inputPath.parent_path();
        std::map<std::filesystem::path, std::filesystem::path> resolvedTexturePaths;

        for (const auto& sourcePath : uniqueTexturePaths)
        {
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
                //texturePathErrors.push_back("Failed to find texture: " + sourcePath.string() + ". " + pathResolution.warning);
                texturePathErrors.push_back(pathResolution.warning);
            }
        }

        if (!texturePathErrors.empty())
        {
            result.errors.push_back("Texture path errors: Could not find one or more textures. Texture compilation aborted entirely.");
            result.errors.insert(result.errors.end(), texturePathErrors.begin(), texturePathErrors.end());
            return;
        }


        compiler::TextureCompiler texCompiler;
        std::filesystem::path textureOutputDir = options.general.outputPath / "textures";
        std::filesystem::create_directories(textureOutputDir);

        for (const auto& [originalPath, resolvedPath] : resolvedTexturePaths)
        {
            CompilerOptions texOpts;
            texOpts.general.inputPath = resolvedPath; // Use the *correct* path
            texOpts.general.outputPath = textureOutputDir;
            texOpts.texture = options.texture;

            // Parse filename to set specific options - e.g: normal maps use BC5
            //Disable parsing for now - ryans vk::Format vk::vkFormatToFormat(VkFormat format) doesn't support any BC5s and BC4s
            //texOpts.texture = GetTextureOptionsForFile(resolvedPath, options.texture);

            if (texCompiler.Compile(texOpts))
            {
                std::string outputFilename = originalPath.stem().string() + ".ktx2";
                result.createdTextureFiles.push_back(options.general.outputPath / "textures" / outputFilename);
            }
            else
            {
                result.errors.push_back("Failed to compile texture: " + resolvedPath.string());
            }
        }
    }

    void SceneCompiler::SaveMeshes(const Scene& scene, CompilationResult& result)
    {
        std::vector<MeshNode> finalNodes;
		std::vector<MeshInfo> finalMeshInfos;
		std::string materialNames; // String with names separated by \0s
		std::vector<uint32_t> finalIndices;
		std::vector<Vertex> finalVertices;

		// Put material names into lookup map
		std::map<uint32_t, uint32_t> materialIndexToNameOffset;
		uint32_t currentNameOffset = 0;
		for (uint32_t i = 0; i < scene.materials.size(); ++i)
		{
			materialIndexToNameOffset[i] = currentNameOffset;
			materialNames += scene.materials[i].name;
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

			finalMeshInfos.push_back(meshInfo);

			finalVertices.insert(finalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());

			for (uint32_t index : mesh.indices)
			{
				finalIndices.push_back(index + vertexOffset);
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

		// Bounds calculation would be placed here.
        // Completely unused right now, but may be a performance improvement in the future

		// Populate file header
		MeshFileHeader header;
		header.magic = MESH_FILE_MAGIC;
		header.numNodes = static_cast<uint32_t>(finalNodes.size());
		header.numMeshes = static_cast<uint32_t>(finalMeshInfos.size());
		header.totalIndices = static_cast<uint32_t>(finalIndices.size());
		header.totalVertices = static_cast<uint32_t>(finalVertices.size());
		header.materialNameBufferSize = static_cast<uint32_t>(materialNames.size());
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

		header.materialNamesOffset = currentOffset;
		currentOffset += materialNames.size(); // Size of the packed string buffer

		header.indexDataOffset = currentOffset;
		currentOffset += finalIndices.size() * sizeof(uint32_t);

		header.vertexDataOffset = currentOffset;


        std::filesystem::path meshOutputDir = options.general.outputPath / "meshes";
        std::filesystem::create_directories(meshOutputDir);
		std::filesystem::path outFilePath = options.general.outputPath / (options.general.inputPath.stem().string() + ".mesh");
		std::ofstream outFile(outFilePath, std::ios::binary);

		//Write the actual data to file
		outFile.write(reinterpret_cast<const char*>(&header), sizeof(MeshFileHeader));

		outFile.write(reinterpret_cast<const char*>(finalNodes.data()), finalNodes.size() * sizeof(MeshNode));
		outFile.write(reinterpret_cast<const char*>(finalMeshInfos.data()), finalMeshInfos.size() * sizeof(MeshInfo));
		outFile.write(materialNames.c_str(), materialNames.size());
		outFile.write(reinterpret_cast<const char*>(finalIndices.data()), finalIndices.size() * sizeof(uint32_t));
		outFile.write(reinterpret_cast<const char*>(finalVertices.data()), finalVertices.size() * sizeof(Vertex));

		outFile.close();

        result.createdMeshFiles.push_back(outFilePath);
    }

    void SceneCompiler::SaveMaterialData(const Scene& scene, CompilationResult& result)
    {
        std::filesystem::path materialOutputDir = options.general.outputPath / "materials";
        std::filesystem::create_directories(materialOutputDir);

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


            const char* textureKeys[] = { "baseColor", "metallicRoughness", "normal", "emissive", "occlusion" };
            rapidjson::Value texturesArray(rapidjson::kArrayType);

            for (const char* key : textureKeys)
            {
                rapidjson::Value textureEntry(rapidjson::kObjectType);
                textureEntry.AddMember("key", rapidjson::Value(key, allocator), allocator);

                std::string valueStr = "";
                auto it = materialSlot.texturePaths.find(key);
                if (it != materialSlot.texturePaths.end())
                {
                    std::string filename = it->second.stem().string() + ".ktx2";    
                    valueStr = "compiledassets/textures/" + filename;
                }

                textureEntry.AddMember("value", rapidjson::Value(valueStr.c_str(), allocator), allocator);
                texturesArray.PushBack(textureEntry, allocator);
            }
            doc.AddMember("textures", texturesArray, allocator);

            doc.AddMember("flags", materialSlot.flags, allocator);

            // Write to disk
            //std::string safeFilename = materialSlot.name;
            //std::replace(safeFilename.begin(), safeFilename.end(), ' ', '_');
            //std::filesystem::path outFilePath = materialOutputDir / (safeFilename + ".material");
            std::filesystem::path outFilePath = materialOutputDir / (materialSlot.name + ".material");

            std::ofstream outFile(outFilePath);
            rapidjson::OStreamWrapper osw(outFile);

            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);

            outFile.close();

            result.createdMaterialFiles.push_back(outFilePath);
        }
    }
}