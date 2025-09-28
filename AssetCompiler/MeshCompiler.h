#pragma once
#include "CompileOptions.h"
#include "SceneLoader.h"
#include "MeshFileStructure.h"
#include <memory>

namespace compiler
{
	struct MeshCompilerOptions
	{
		CommonCompileOptions commonOptions;
	};

	class MeshCompiler
	{
	public:
		MeshCompiler() = default;
		~MeshCompiler() = default;

		bool Compile(const MeshCompilerOptions& compileOptions);

	private:
		void SaveMeshFile(Scene& scene);
		SceneLoader sceneLoader;

		MeshCompilerOptions options;
	};

	//std::unique_ptr<LoadedScene> LoadMeshAsset(const std::filesystem::path& path);
}


