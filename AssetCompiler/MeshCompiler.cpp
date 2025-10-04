//#include "MeshCompiler.h"
//#include <fstream>
//#include <map>
//
//namespace compiler
//{
//
//	bool MeshCompiler::Compile(const MeshCompilerOptions& compileOptions)
//	{
//		options = compileOptions;
//
//		SceneLoadResult sceneResult = sceneLoader.loadScene(options.commonOptions.inputPath);
//
//		if (!sceneResult.success)
//		{
//			return false;
//		}
//
//		SaveMeshFile(sceneResult.scene);
//
//		return true;
//	}
//	void MeshCompiler::SaveMeshFile(Scene& scene)
//	{
//		std::vector<MeshNode> finalNodes;
//		std::vector<MeshInfo> finalMeshInfos;
//		std::string materialNames; // A single string with names separated by null terminators
//		std::vector<uint32_t> finalIndices;
//		std::vector<Vertex> finalVertices;
//
//		// Put material names into lookup map
//		std::map<uint32_t, uint32_t> materialIndexToNameOffset;
//		uint32_t currentNameOffset = 0;
//		for (uint32_t i = 0; i < scene.materials.size(); ++i)
//		{
//			materialIndexToNameOffset[i] = currentNameOffset;
//			materialNames += scene.materials[i].name;
//			materialNames += '\0';
//			currentNameOffset = materialNames.size();
//		}
//		
//
//		//Set up Mesh vertex and index buffers
//		uint32_t vertexOffset = 0;
//		uint32_t indexOffset = 0;
//
//		for (const auto& mesh : scene.meshes)
//		{
//			MeshInfo meshInfo;
//			meshInfo.firstIndex = indexOffset;
//			meshInfo.indexCount = mesh.indices.size();
//			meshInfo.firstVertex = vertexOffset;
//			meshInfo.materialNameIndex = materialIndexToNameOffset[mesh.materialIndex];
//			meshInfo.meshBounds = mesh.bounds;
//
//			finalMeshInfos.push_back(meshInfo);
//
//			finalVertices.insert(finalVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
//
//			for (uint32_t index : mesh.indices)
//			{
//				finalIndices.push_back(index + vertexOffset);
//			}
//
//			vertexOffset += mesh.vertices.size();
//			indexOffset += mesh.indices.size();
//		}
//
//		for (const auto& compilerNode : scene.nodes)
//		{
//			MeshNode node;
//			node.transform = compilerNode.localTransform;
//			node.meshIndex = compilerNode.meshIndex;
//			node.parentIndex = compilerNode.parentIndex;
//
//			strncpy_s(node.name, sizeof(node.name), compilerNode.name.c_str(), sizeof(node.name) - 1);
//			node.name[sizeof(node.name) - 1] = '\0';
//
//			finalNodes.push_back(node);
//		}
//
//		// Bounds calculation??????
//		// Transform calculation???
//
//		// Populate file header
//		MeshFileHeader header;
//		header.magic = MESH_FILE_MAGIC;
//		//header.version = 1;
//		header.numNodes = finalNodes.size();
//		header.numMeshes = finalMeshInfos.size();
//		header.totalIndices = finalIndices.size();
//		header.totalVertices = finalVertices.size();
//		header.materialNameBufferSize = materialNames.size();
//		header.sceneBoundsCenter = scene.center;
//		header.sceneBoundsRadius = scene.radius;
//		header.sceneBoundsMin = scene.boundingMin;
//		header.sceneBoundsMax = scene.boundingMax;
//
//
//		// Calculate data offsets
//		uint64_t currentOffset = sizeof(MeshFileHeader);
//		header.nodeDataOffset = currentOffset;
//		currentOffset += finalNodes.size() * sizeof(MeshNode);
//
//		header.meshInfoDataOffset = currentOffset;
//		currentOffset += finalMeshInfos.size() * sizeof(MeshInfo);
//
//		header.materialNamesOffset = currentOffset;
//		currentOffset += materialNames.size(); // Size of the packed string buffer
//
//		header.indexDataOffset = currentOffset;
//		currentOffset += finalIndices.size() * sizeof(uint32_t);
//
//		header.vertexDataOffset = currentOffset;
//
//		std::filesystem::path outFilePath = options.commonOptions.outputPath / (options.commonOptions.inputPath.stem().string() + ".mesh");
//		std::ofstream outFile(outFilePath, std::ios::binary);
//
//		//Write the actual data to file
//		outFile.write(reinterpret_cast<const char*>(&header), sizeof(MeshFileHeader));
//
//		outFile.write(reinterpret_cast<const char*>(finalNodes.data()), finalNodes.size() * sizeof(MeshNode));
//		outFile.write(reinterpret_cast<const char*>(finalMeshInfos.data()), finalMeshInfos.size() * sizeof(MeshInfo));
//		outFile.write(materialNames.c_str(), materialNames.size());
//		outFile.write(reinterpret_cast<const char*>(finalIndices.data()), finalIndices.size() * sizeof(uint32_t));
//		outFile.write(reinterpret_cast<const char*>(finalVertices.data()), finalVertices.size() * sizeof(Vertex));
//
//		outFile.close();
//	}
//	
//	/*
//	std::unique_ptr<LoadedScene> LoadMeshAsset(const std::filesystem::path& path)
//	{
//		// 1. Open the file for reading in binary mode
//		std::ifstream file(path, std::ios::binary);
//		if (!file.is_open())
//		{
//			return nullptr;
//		}
//
//		// 2. Read the header from the start of the file
//		FileHeader header;
//		file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
//
//		// 3. Validate the header's magic number
//		// This is a crucial check to ensure you're reading the right file type.
//		if (header.magic != 'HSAM') // 'MESH' backwards
//		{
//			file.close();
//			return nullptr;
//		}
//
//		auto scene = std::make_unique<LoadedScene>();
//
//		// 4. Resize vectors to their final sizes
//		scene->nodes.resize(header.numNodes);
//		scene->meshInfos.resize(header.numMeshes);
//		scene->materialNamesBuffer.resize(header.materialNameBufferSize);
//		scene->indices.resize(header.totalIndices);
//		scene->vertices.resize(header.totalVertices);
//		// You can also copy the bounds from the header to the scene here
//		// scene->center = header.sceneBoundsCenter;
//
//		// 5. Read all the data blobs directly into the vectors
//		// We use the offsets from the header to seek to the correct position for each block.
//
//		// Read Nodes
//		file.seekg(header.nodeDataOffset);
//		file.read(reinterpret_cast<char*>(scene->nodes.data()), header.numNodes * sizeof(FileNode));
//
//		// Read Mesh Info
//		file.seekg(header.meshInfoDataOffset);
//		file.read(reinterpret_cast<char*>(scene->meshInfos.data()), header.numMeshes * sizeof(FileMeshInfo));
//
//		// Read Material Names Buffer
//		file.seekg(header.materialNamesOffset);
//		file.read(scene->materialNamesBuffer.data(), header.materialNameBufferSize);
//
//		// Read Indices
//		file.seekg(header.indexDataOffset);
//		file.read(reinterpret_cast<char*>(scene->indices.data()), header.totalIndices * sizeof(uint32_t));
//
//		// Read Vertices
//		file.seekg(header.vertexDataOffset);
//		file.read(reinterpret_cast<char*>(scene->vertices.data()), header.totalVertices * sizeof(Vertex));
//
//		// 6. Close the file and return the loaded scene
//		file.close();
//
//		std::cout << "Scene loaded!";
//		
//		return scene;
//	}
//	*/
//}