#include "CompileOptions.h"
#include "SceneCompiler.h"
#include "TextureCompiler.h"
#include "CmdlineParser.h"

using namespace compiler;

// --- Helper: String to Enum ---
TextureCompressionFormat ParseCompressionFormat(const std::string& fmt)
{
    std::string upper = fmt;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "BC1") return TextureCompressionFormat::BC1;
    if (upper == "BC3") return TextureCompressionFormat::BC3;
    if (upper == "BC4") return TextureCompressionFormat::BC4;
    if (upper == "BC5") return TextureCompressionFormat::BC5;
    if (upper == "BC7") return TextureCompressionFormat::BC7;
    if (upper == "ASTC") return TextureCompressionFormat::ASTC;
    if (upper == "ETC") return TextureCompressionFormat::ETC;

    return TextureCompressionFormat::UNCOMPRESSED;
}

void SetupCLIArguments(Parser& parser)
{
    // General
    parser.AddOption("input", "Path to source asset (fbx, png, etc.)", true, 1);
    parser.AddOption("output", "Directory to output compiled assets", true, 1);
    parser.AddOption("platform", "Target Platform (windows/android)", false, 1);

    // Mesh Options - but if you dont wanna optimise it, then what's the point of doing this even?
    //parser.AddOption("no-opt", "Disable mesh optimization", false, 0);
    //parser.AddOption("no-tangents", "Skip tangent generation", false, 0);
    parser.AddOption("noflip-uv", "Don't flip UV coordinates", false, 0);

    // Texture Options
    parser.AddOption("format", "Compression format (BC1, BC3, BC7, ASTC, ETC)", false, 1);
    parser.AddOption("mipcount", "Mipmap generation count", false, 1);
    parser.AddOption("quality", "Compression quality (0.0 - 1.0)", false, 1);
}

int main(int argc, char* argv[])
{
    Parser parser;
    SetupCLIArguments(parser);

    if (!parser.Parse(argc, argv))
    {
        parser.PrintHelp();
        return 1; // Error code
    }

    auto inputStr = parser.GetOption<std::string>("input");
    auto outputStr = parser.GetOption<std::string>("output");

    CompilerOptions options;

    // General options
    options.general.inputPath = inputStr.value();
    options.general.outputPath = outputStr.value();

    //options.mesh.optimize = !parser.HasOption("no-opt");
    //options.mesh.generateTangents = !parser.HasOption("no-tangents");
    options.mesh.flipUVs = !parser.HasOption("flip-uv"); //We flip by default anyway

    // Texture Defaults
    if (auto count = parser.GetOption<int>("mipcount"))
    {
        if (count.value() > 1)
        {
            options.texture.generateMipmaps = true;
        }
        options.texture.mipCount = count.value();
    }

    // Quality
    if (auto q = parser.GetOption<double>("quality"))
        options.texture.quality = static_cast<float>(q.value());






}