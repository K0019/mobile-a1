#include "TextureCompiler.h"
#include "MeshCompiler.h"
#include "AssetCompiler.h"

namespace compiler
{

    bool compiler::CompileMeshSimple(std::filesystem::path filepath, std::filesystem::path output)
    {
        MeshCompiler meshCompiler;
        MeshCompilerOptions options;
        options.commonOptions.inputPath = filepath;
        options.commonOptions.outputPath = output;
        return meshCompiler.Compile(options);
    }

    bool CompileTextureSimple(std::filesystem::path filepath, std::filesystem::path output)
    {
        //im not ready....
        return false;
    }
}
