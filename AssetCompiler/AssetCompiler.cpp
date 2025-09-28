#include "TextureCompiler.h"
#include "MeshCompiler.h"

/*

int main(int argc, const char* argv[])
{

    compiler::CompileOptions options;
    options.inputPath = "FakeAssets\\hatchback-sports.fbx";
    options.outputPath = "CompiledOutput\\";
    static const char* debugArgsTexture[] =
    { "TextureCompiler"
    , "-input"
    , "FakeAssets\\ruby.png"
    , "-output"
    , "CompiledOutput\\"
    };

    argv = debugArgsTexture;
    argc = static_cast<int>(sizeof(debugArgsTexture) / sizeof(debugArgsTexture[0]));

    compiler::TextureCompiler textureCompiler;
    textureCompiler.SetupCommandLine();
    auto msg = textureCompiler.Parse(argc, argv);
    if (msg != compiler::CompilerBase::MESSAGE::OK)
    {
        std::cout << "Parsing failed!";
        return -1;
    }
    textureCompiler.Compile();

    static const char* debugArgsMesh[] =
    { "MeshCompiler"
    , "-input"
    , "FakeAssets\\ambulance.fbx"
    , "-output"
    , "CompiledOutput\\"
    };
    argv = debugArgsMesh;
    argc = static_cast<int>(sizeof(debugArgsMesh) / sizeof(debugArgsMesh[0]));

    compiler::MeshCompiler meshCompiler;
    meshCompiler.SetupCommandLine();
    //auto msg = meshCompiler.Parse(argc, argv);
    //if (msg != compiler::CompilerBase::MESSAGE::OK)
    //{
    //    std::cout << "Mesh Parsing failed!";
    //    return -1;
    //}
    meshCompiler.Compile(options);

    Scene outScene;
    auto scene = compiler::LoadMeshAsset("CompiledOutput\\hatchback-sports.mesh");

    std::cout << scene->vertices.size() << "\n";
    std::cout << scene->indices.size() << "\n";
    for (auto c : scene->materialNamesBuffer)
    {
        std::cout << c;
    }std::cout << ": ";
    std::cout << scene->materialNamesBuffer.size() << "\n";
    std::cout << scene->meshInfos.size() << "\n";
    std::cout << scene->nodes.size() << "\n";

}
    */





//Test cases
#if 0

void testBasicFunctionality()
{
    std::cout << "=== Testing Basic Functionality ===\n";

    compiler::Parser parser;

    // Add various types of options
    parser.AddOption("input", "Input file path", true, 1);      // required, 1 arg
    parser.AddOption("output", "Output file path", false, 1);   // optional, 1 arg
    parser.AddOption("verbose", "Enable verbose output", false, 0);  // flag only
    parser.AddOption("count", "Number of iterations", false, 1);     // optional, 1 arg
    parser.AddOption("values", "List of values", false, 2);          // optional, min 2 args

    // Test 1: Successful parsing with all options
    {
        std::cout << "\nTest 1: Full command line\n";
        const char* argv[] = { "test_program", "-input", "file.txt", "--output", "out.txt",
                             "-verbose", "--count", "42", "-values", "1.5", "2.7", "3.14" };
        int argc = sizeof(argv) / sizeof(argv[0]);

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success: " << (success ? "true" : "false") << "\n";

        // Test HasOption
        std::cout << "Has input: " << parser.HasOption("input") << "\n";
        std::cout << "Has output: " << parser.HasOption("output") << "\n";
        std::cout << "Has verbose: " << parser.HasOption("verbose") << "\n";
        std::cout << "Has count: " << parser.HasOption("count") << "\n";
        std::cout << "Has values: " << parser.HasOption("values") << "\n";
        std::cout << "Has nonexistent: " << parser.HasOption("nonexistent") << "\n";

        // Test GetOption for single values
        auto input = parser.GetOption<std::string>("input");
        auto output = parser.GetOption<std::string>("output");
        auto count = parser.GetOption<int64_t>("count");

        std::cout << "Input: " << (input ? *input : "not found") << "\n";
        std::cout << "Output: " << (output ? *output : "not found") << "\n";
        std::cout << "Count: " << (count ? std::to_string(*count) : "not found") << "\n";

        // Test GetOptions for multiple values
        auto values = parser.GetOptions<double>("values");
        std::cout << "Values (" << values.size() << "): ";
        for (double v : values)
        {
            std::cout << v << " ";
        }
        std::cout << "\n";

        // Test GetOptionArgs for raw strings
        auto rawValues = parser.GetOptionArgs("values");
        std::cout << "Raw values (" << rawValues.size() << "): ";
        for (const auto& v : rawValues)
        {
            std::cout << "\"" << v << "\" ";
        }
        std::cout << "\n";
    }

    parser.Clear();
}

void testRequiredOptions()
{
    std::cout << "\n=== Testing Required Options ===\n";

    compiler::Parser parser;
    parser.AddOption("input", "Input file", true, 1);
    parser.AddOption("output", "Output file", false, 1);

    // Test 2: Missing required option
    {
        std::cout << "\nTest 2: Missing required option\n";
        const char* argv[] = { "test_program", "-output", "out.txt" };
        int argc = sizeof(argv) / sizeof(argv[0]);

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success (should be false): " << (success ? "true" : "false") << "\n";
    }

    // Test 3: Required option present
    {
        std::cout << "\nTest 3: Required option present\n";
        const char* argv[] = { "test_program", "-input", "in.txt", "-output", "out.txt" };
        int argc = sizeof(argv) / sizeof(argv[0]);

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success (should be true): " << (success ? "true" : "false") << "\n";
    }

    parser.Clear();
}

void testMinimumArguments()
{
    std::cout << "\n=== Testing Minimum Arguments ===\n";

    compiler::Parser parser;
    parser.AddOption("coords", "X Y coordinates", false, 2); // needs at least 2 args

    // Test 4: Not enough arguments
    {
        std::cout << "\nTest 4: Not enough arguments\n";
        const char* argv[] = { "test_program", "-coords", "10" };
        int argc = sizeof(argv) / sizeof(argv[0]);

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success (should be false): " << (success ? "true" : "false") << "\n";
    }

    // Test 5: Enough arguments
    {
        std::cout << "\nTest 5: Enough arguments\n";
        const char* argv[] = { "test_program", "-coords", "10", "20", "30" };
        int argc = sizeof(argv) / sizeof(argv[0]);

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success (should be true): " << (success ? "true" : "false") << "\n";

        auto coords = parser.GetOptions<int64_t>("coords");
        std::cout << "Coordinates (" << coords.size() << "): ";
        for (int64_t c : coords)
        {
            std::cout << c << " ";
        }
        std::cout << "\n";
    }

    parser.Clear();
}

void testTypeConversion()
{
    std::cout << "\n=== Testing Type Conversion ===\n";

    compiler::Parser parser;
    parser.AddOption("number", "A number", false, 1);
    parser.AddOption("decimal", "A decimal", false, 1);
    parser.AddOption("bad", "Bad number", false, 1);

    const char* argv[] = { "test_program", "-number", "42", "-decimal", "3.14159", "-bad", "not_a_number" };
    int argc = sizeof(argv) / sizeof(argv[0]);

    bool success = parser.Parse(argc, argv);
    std::cout << "Parse success: " << (success ? "true" : "false") << "\n";

    // Test successful conversions
    auto number = parser.GetOption<int64_t>("number");
    auto decimal = parser.GetOption<double>("decimal");
    auto badNumber = parser.GetOption<int64_t>("bad");

    std::cout << "Number as int64_t: " << (number ? std::to_string(*number) : "conversion failed") << "\n";
    std::cout << "Decimal as double: " << (decimal ? std::to_string(*decimal) : "conversion failed") << "\n";
    std::cout << "Bad as int64_t: " << (badNumber ? std::to_string(*badNumber) : "conversion failed") << "\n";

    // Test getting as string (should always work)
    auto badAsString = parser.GetOption<std::string>("bad");
    std::cout << "Bad as string: " << (badAsString ? *badAsString : "not found") << "\n";

    parser.Clear();
}

void testHelpMessage()
{
    std::cout << "\n=== Testing Help Message ===\n";

    compiler::Parser parser;
    parser.AddOption("input", "Input file path", true, 1);
    parser.AddOption("output", "Output file path", false, 1);
    parser.AddOption("verbose", "Enable verbose output");
    parser.AddOption("count", "Number of iterations", false, 1);
    parser.AddOption("coords", "X Y coordinates", false, 2);

    // Set a fake program name
    const char* argv[] = { "my_program" };
    parser.Parse(1, argv);

    std::cout << "\nHelp message:\n";
    parser.PrintHelp();

    parser.Clear();
}

void testEdgeCases()
{
    std::cout << "\n=== Testing Edge Cases ===\n";

    compiler::Parser parser;
    parser.AddOption("flag", "A simple flag", false, 0);

    // Test 6: Empty command line (just program name)
    {
        std::cout << "\nTest 6: Empty command line\n";
        const char* argv[] = { "test_program" };
        int argc = 1;

        bool success = parser.Parse(argc, argv);
        std::cout << "Parse success: " << (success ? "true" : "false") << "\n";
        std::cout << "Has flag: " << parser.HasOption("flag") << "\n";
    }

    // Test 7: Multiple flag formats
    {
        std::cout << "\nTest 7: Different flag formats\n";
        parser.Clear();
        parser.AddOption("test", "Test flag", false, 0);

        // Test -flag format
        const char* argv1[] = { "prog", "-test" };
        if (parser.Parse(2, argv1))
        {
            std::cout << "Single dash format works: " << parser.HasOption("test") << "\n";
        }

        parser.Clear();
        parser.AddOption("test", "Test flag", false, 0);

        // Test --flag format
        const char* argv2[] = { "prog", "--test" };
        if (parser.Parse(2, argv2))
        {
            std::cout << "Double dash format works: " << parser.HasOption("test") << "\n";
        }
    }
}


#endif 