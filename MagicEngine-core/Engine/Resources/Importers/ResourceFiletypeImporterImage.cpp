#include "Engine/Resources/Importers/ResourceFiletypeImporterImage.h"
// Image files are compiled to .ktx2, so we need to pass that new file back to ResourceImporter to fully import.
#ifdef GLFW
#include "TextureCompiler.h"
#endif
#include "Engine/Resources/ResourceImporter.h"
#include "FilepathConstants.h"

bool ResourceFiletypeImporterImage::Import([[maybe_unused]]const std::string& assetRelativeFilepath)
{
#ifdef GLFW
    // Set up compile options
    compiler::TextureCompiler textureCompiler;
    compiler::CompilerOptions options;
    options.general.inputPath = GetExeRelativeFilepath(assetRelativeFilepath);
    options.general.outputPath = Filepaths::assets + "/CompiledAssets/textures";

    // Compile into .ktx2
    auto texCompilerResult = textureCompiler.Compile(options);
    if (!texCompilerResult.errors.empty())
    {
        for (const auto& error : texCompilerResult.errors)
        {
            CONSOLE_LOG(LEVEL_ERROR) << error;
        }
        
        return false;
    }

    // Import the .ktx2
    std::string filename = (options.general.outputPath / options.general.inputPath.stem()).string();
    filename += ".ktx2";
    return ResourceImporter::Import(VFS::ConvertPhysicalToVirtual(filename));
#else
	CONSOLE_LOG_UNIMPLEMENTED() << "Importing images is not implemented for this platform.";
	return false;
#endif
}
