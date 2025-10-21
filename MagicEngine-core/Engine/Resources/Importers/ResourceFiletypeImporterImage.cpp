#include "Engine/Resources/Importers/ResourceFiletypeImporterImage.h"
// Image files are compiled to .ktx2, so we need to pass that new file back to ResourceImporter to fully import.
#ifdef GLFW
#include "TextureCompiler.h"
#endif
#include "Engine/Resources/ResourceImporter.h"
#include "FilepathConstants.h"

bool ResourceFiletypeImporterImage::Import(const std::string& assetRelativeFilepath)
{
#ifdef GLFW
    // Set up compile options
    compiler::TextureCompiler textureCompiler;
    compiler::CompilerOptions options;
    options.general.inputPath = GetExeRelativeFilepath(assetRelativeFilepath);
    options.general.outputPath = Filepaths::assets + "/CompiledAssets/textures";

    // Compile into .ktx2
    if (!textureCompiler.Compile(options))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to convert " << options.general.inputPath.string() << " to .ktx2";
        return false;
    }

    // Import the .ktx2
    std::string filename = (options.general.outputPath / options.general.inputPath.stem()).string();
    filename += ".ktx2";
    return ResourceImporter::Import(filename);
#else
	CONSOLE_LOG_UNIMPLEMENTED() << "Importing images is not implemented for this platform.";
	return false;
#endif
}
