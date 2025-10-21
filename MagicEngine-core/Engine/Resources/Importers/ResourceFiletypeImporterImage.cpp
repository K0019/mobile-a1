#include "Engine/Resources/Importers/ResourceFiletypeImporterImage.h"
// Image files are compiled to .ktx2, so we need to pass that new file back to ResourceImporter to fully import.
#include "TextureCompiler.h"
#include "Engine/Resources/ResourceImporter.h"
#include "FilepathConstants.h"

bool ResourceFiletypeImporterImage::Import(const std::filesystem::path& assetRelativeFilepath)
{
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
    return ResourceImporter::Import(options.general.outputPath / options.general.inputPath.stem() += ".ktx2");
}
