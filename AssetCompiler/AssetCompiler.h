#pragma once
#include "CompileOptions.h"
#include "MeshCompiler.h"
#include "TextureCompiler.h"

namespace compiler
{
	bool CompileMeshSimple(std::filesystem::path filepath, std::filesystem::path out);
	bool CompileTextureSimple(std::filesystem::path filepath, std::filesystem::path out);
}