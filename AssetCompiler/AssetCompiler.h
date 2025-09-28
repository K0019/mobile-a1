#pragma once
#include "CompileOptions.h"
#include "MeshCompiler.h"
#include "TextureCompiler.h"

namespace compiler
{

	// Helper functions, just compiles with default settings
	// Output should be the directory to put the compiled asset into.
	// Output will be is output/nameofasset + .filetype
	// No checks for directory exists or file exists
	bool CompileMeshSimple(std::filesystem::path filepath, std::filesystem::path output);
	bool CompileTextureSimple(std::filesystem::path filepath, std::filesystem::path output);
}