# modules/tools/CMakeLists.txt

# Create Tools module (automatically globs all files in current directory)
engine_add_module(ryEngine-Tools
    FOLDER "ryEngine-modules/tools"
    PUBLIC_DEPENDENCIES
        ryEngine-Core
    DEPENDENCIES
        # Asset processing
        assimp::assimp
        freeimage::FreeImage
        freeimage::FreeImagePlus
        KTX::ktx
        mikktspace::mikktspace
        meshoptimizer::meshoptimizer
        # Shader compilation
        glslang::glslang
        glslang::glslang-default-resource-limits
        glslang::SPIRV
        glslang::SPVRemapper
        # Profiling
        Tracy::TracyClient
    DEFINITIONS
        TRACY_ENABLE
)

target_include_directories(ryEngine-Tools
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
