#include "SceneCompiler.h"
#include "MeshProcessor.h"
#include "TextureCompiler.h"
#include "MeshFileStructure.h"

#include <fstream>
#include <set>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/ostreamwrapper.h"

namespace compiler
{
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
        ProcessScene(scene, compileOptions.mesh);

        // Save the data
        SaveMeshes(scene, result);
        //SaveMaterialData(scene, result);
        CompileTextures(scene, result);

        if (!result.errors.empty())
        {
            result.success = false;
        }

        return result;
    }

    SceneProcessingResult SceneCompiler::ProcessScene(Scene& scene, const MeshOptions& options)
    {
        SceneProcessingResult processingResult;
        for (ProcessedMesh& mesh : scene.meshes)
        {
            if (mesh.vertices.empty() || mesh.indices.empty())
            {
                continue;
            }

            if (options.optimize && MeshOptimizer::shouldOptimize(mesh.vertices, mesh.indices))
            {
                auto result = MeshOptimizer::optimize(mesh.vertices, mesh.indices);

                if (!result.success)
                {
                    processingResult.warnings.push_back("MeshOptimizer::optimize failed");
                }
            }

            if (options.generateTangents)
            {
                if (!MeshOptimizer::generateTangents(mesh.vertices, mesh.indices))
                {
                    processingResult.warnings.push_back("MeshOptimizer::generateTangents failed");
                }
            }

            if (options.calculateBounds)
            {
                mesh.bounds = calculateBounds(mesh.vertices);
            }
        }
        return processingResult;
    }

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

        compiler::TextureCompiler texCompiler;

        std::filesystem::path textureOutputDir = options.general.outputPath / "textures";
        std::filesystem::create_directories(textureOutputDir);
        for (const auto& sourcePath : uniqueTexturePaths)
        {
            CompilerOptions texOpts;
            texOpts.general.inputPath = sourcePath;
            texOpts.general.outputPath = textureOutputDir;
            texOpts.texture = options.texture;

            if (texCompiler.Compile(texOpts))
            {
                std::string outputFilename = sourcePath.stem().string() + ".ktx2";
                result.createdTextureFiles.push_back(options.general.outputPath / "textures" / outputFilename);
            }
            else
            {
                result.errors.push_back("Failed to compile texture: " + sourcePath.string());
            }
        }

    }

    void SceneCompiler::SaveMeshes(const Scene& scene, CompilationResult& result)
    {
        // TODO: Loop through scene.meshes and save each one to a separate .mesh file.

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
			currentNameOffset = materialNames.size();
		}
		

		//Set up Mesh vertex and index buffers
		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;

		for (const auto& mesh : scene.meshes)
		{
			MeshInfo meshInfo;
			meshInfo.firstIndex = indexOffset;
			meshInfo.indexCount = mesh.indices.size();
			meshInfo.firstVertex = vertexOffset;
			meshInfo.materialNameIndex = materialIndexToNameOffset[mesh.materialIndex];
			meshInfo.meshBounds = mesh.bounds;

			finalMeshInfos.push_back(meshInfo);

			finalVertices.insert(finalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());

			for (uint32_t index : mesh.indices)
			{
				finalIndices.push_back(index + vertexOffset);
			}

			vertexOffset += mesh.vertices.size();
			indexOffset += mesh.indices.size();
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

		// Bounds calculation??????
		// Transform calculation???

		// Populate file header
		MeshFileHeader header;
		header.magic = MESH_FILE_MAGIC;
		//header.version = 1;
		header.numNodes = finalNodes.size();
		header.numMeshes = finalMeshInfos.size();
		header.totalIndices = finalIndices.size();
		header.totalVertices = finalVertices.size();
		header.materialNameBufferSize = materialNames.size();
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

            if (!materialSlot.texturePaths.empty())
            {
                rapidjson::Value texturesJson(rapidjson::kObjectType);
                for (const auto& [type, path] : materialSlot.texturePaths)
                {
                    std::string filename = path.filename().string();

                    rapidjson::Value key(type.c_str(), allocator);
                    rapidjson::Value value(filename.c_str(), allocator);

                    texturesJson.AddMember(key, value, allocator);
                }
                doc.AddMember("textures", texturesJson, allocator);
            }

            if (!materialSlot.scalarParams.empty() || !materialSlot.vectorParams.empty())
            {
                rapidjson::Value paramsJson(rapidjson::kObjectType);
                for (const auto& [name, value] : materialSlot.scalarParams)
                {
                    paramsJson.AddMember(rapidjson::Value(name.c_str(), allocator), rapidjson::Value(static_cast<double>(value)), allocator);
                }
                for (const auto& [name, value] : materialSlot.vectorParams)
                {
                    rapidjson::Value vecArray(rapidjson::kArrayType);
                    vecArray.PushBack(value.x, allocator);
                    vecArray.PushBack(value.y, allocator);
                    vecArray.PushBack(value.z, allocator);
                    vecArray.PushBack(value.w, allocator);

                    paramsJson.AddMember(rapidjson::Value(name.c_str(), allocator), vecArray, allocator);
                }
                doc.AddMember("parameters", paramsJson, allocator);
            }

            std::string safeFilename = materialSlot.name;
            std::replace(safeFilename.begin(), safeFilename.end(), ' ', '_');
            std::filesystem::path outFilePath = materialOutputDir / (safeFilename + ".material");

            std::ofstream outFile(outFilePath);
            rapidjson::OStreamWrapper osw(outFile);

            rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
            doc.Accept(writer);

            outFile.close();

            result.createdMaterialFiles.push_back(outFilePath);
        }
    }
}