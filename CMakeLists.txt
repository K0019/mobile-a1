cmake_minimum_required(VERSION 3.22)

include(cmake/EngineUtils.cmake)
set(namespace "MagicEngine")

# ----- VCPKG Configuration (minimal - only for complex desktop deps) -----
# MUST be before project() for toolchain to work
if(NOT ANDROID)
    set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg")
    if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    endif()

    # Bootstrap vcpkg if needed
    if(EXISTS "${VCPKG_ROOT}" AND NOT EXISTS "${VCPKG_ROOT}/vcpkg.exe" AND WIN32)
        execute_process(COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.bat" "-disableMetrics" RESULT_VARIABLE VCPKG_BOOTSTRAP_RESULT)
    endif()

    set(VCPKG_MANIFEST_MODE ON)
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# ----- Project Definition -----
project(MagicEngine VERSION 0.1.0 LANGUAGES C CXX)

if(NOT ANDROID)
    message(STATUS "Using vcpkg for complex dependencies (assimp, freeimage)")
endif()

# ----- External Libraries Configuration ------
include(GNUInstallDirs)

set(EXTERNAL_INCLUDES "${CMAKE_SOURCE_DIR}/extern/include")
set(EXTERNAL_INCLUDES_INSTALL "${CMAKE_INSTALL_INCLUDEDIR}")
set(EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/extern/lib")

# Force install dirs to be relative for cross-compilation (Android)
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "Include directory" FORCE)
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Library directory" FORCE)
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_BINDIR}")
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "Binary directory" FORCE)
endif()

# ----- C++ Standard -----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS OFF)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Filename postfix for libraries under DEBUG configuration")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ----- Global Compile Definitions -----
add_definitions("-DQUILL_DISABLE_NON_PREFIXED_MACROS")
add_definitions("-DVK_NO_PROTOTYPES=1")
if(ANDROID)
    add_definitions("-DVK_USE_PLATFORM_ANDROID_KHR=1")
elseif(WIN32)
    add_definitions("-DVK_USE_PLATFORM_WIN32_KHR=1")
    add_definitions("-DNOMINMAX")
endif()

if(MSVC)
    add_compile_options(
        /W4 /MP /FS
        /wd4828 /wd4819 /wd4505 /wd4324
    )
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
endif()

message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
if(ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
endif()

# =============================================================================
# SUBMODULE DEPENDENCIES
# =============================================================================

message(STATUS "")
message(STATUS "=== Configuring Submodule Dependencies ===")

# ----- Submodule Initialization Check -----
# Check if critical submodules are initialized. If not, provide clear instructions.
set(REQUIRED_SUBMODULES
    Vulkan-Headers
    volk
    VulkanMemoryAllocator
    glm
    fmt
    glslang
    imgui
    glfw
)

set(MISSING_SUBMODULES "")
foreach(SUBMOD ${REQUIRED_SUBMODULES})
    set(SUBMOD_MARKER "CMakeLists.txt")
    if(SUBMOD STREQUAL "imgui")
        set(SUBMOD_MARKER "imgui.h")
    endif()
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/${SUBMOD}/${SUBMOD_MARKER}")
        list(APPEND MISSING_SUBMODULES ${SUBMOD})
    endif()
endforeach()

if(MISSING_SUBMODULES)
    message(FATAL_ERROR
        "\n"
        "================================================================================\n"
        "ERROR: Git submodules not initialized!\n"
        "================================================================================\n"
        "\n"
        "This branch uses git submodules for dependencies.\n"
        "Missing submodules: ${MISSING_SUBMODULES}\n"
        "\n"
        "To fix this, run in your terminal:\n"
        "\n"
        "    git submodule update --init --recursive\n"
        "\n"
        "If you get 'fatal: not a git repository' errors when switching branches,\n"
        "run these commands instead:\n"
        "\n"
        "    git submodule deinit --all -f\n"
        "    git submodule update --init --recursive\n"
        "\n"
        "Or on Windows, run: scripts\\setup_submodules.bat\n"
        "\n"
        "================================================================================\n"
    )
endif()

# ----- Vulkan Headers (must be first) -----
add_subdirectory(extern/Vulkan-Headers)

# ----- Volk (Vulkan loader) -----
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/volk)
# Link volk to Vulkan headers since we disabled VOLK_PULL_IN_VULKAN
target_link_libraries(volk PUBLIC Vulkan::Headers)

# ----- VulkanMemoryAllocator -----
set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "" FORCE)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON CACHE BOOL "" FORCE)
add_subdirectory(extern/VulkanMemoryAllocator)

# ----- GLM -----
add_subdirectory(extern/glm)

# ----- fmt -----
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/fmt)

# ----- Quill (logging) -----
set(QUILL_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
set(QUILL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QUILL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/quill)

# ----- SPIRV-Cross -----
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(extern/SPIRV-Cross)

# ----- SPIRV-Reflect -----
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
add_subdirectory(extern/SPIRV-Reflect)

# ----- SPIRV-Headers (required by SPIRV-Tools) -----
add_subdirectory(extern/SPIRV-Headers)

# ----- SPIRV-Tools (required by glslang optimizer) -----
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/SPIRV-Tools)

# ----- glslang -----
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ALLOW_EXTERNAL_SPIRV_TOOLS ON CACHE BOOL "" FORCE)
add_subdirectory(extern/glslang)

# ----- FreeType -----
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory(extern/freetype)

# ----- JoltPhysics -----
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)  # Disable Vulkan compute - conflicts with volk
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)  # Use dynamic runtime /MD
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)  # Enable RTTI for JoltDebugRenderer inheritance
add_subdirectory(extern/JoltPhysics/Build)

# ----- Recast Navigation -----
set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "" FORCE)
set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/recastnavigation)

# ----- Lua (wrapper) -----
include(extern/cmake/lua.cmake)

# ----- LuaBridge3 (after Lua) -----
set(LUABRIDGE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/LuaBridge3)

# ----- Desktop-only dependencies -----
if(NOT ANDROID)
    # GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/glfw)

    # Tracy
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
    add_subdirectory(extern/tracy)

    # meshoptimizer
    set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "" FORCE)
    set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(extern/meshoptimizer)

    # vcpkg dependencies (complex ones only)
    find_package(Boost CONFIG REQUIRED)
    find_package(assimp CONFIG REQUIRED)
    find_package(FreeImage CONFIG REQUIRED)
    find_package(KTX CONFIG REQUIRED)
    find_package(mikktspace CONFIG REQUIRED)
endif()

# For Android, create a dummy Boost target (boost-type-index is header-only, can be replaced)
if(ANDROID)
    add_library(Boost::boost INTERFACE IMPORTED)
endif()

# ----- ImGui and related (wrappers - after GLFW/freetype) -----
include(extern/cmake/imgui.cmake)
include(extern/cmake/imguizmo.cmake)
include(extern/cmake/implot.cmake)

# ----- KTX-Software (for Android) -----
if(ANDROID)
    set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BASISU_SUPPORT_SSE OFF CACHE BOOL "" FORCE)

    set(SAVED_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
    set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "" FORCE)
    add_subdirectory(extern/KTX-Software)
    set(CMAKE_DEBUG_POSTFIX ${SAVED_DEBUG_POSTFIX} CACHE STRING "" FORCE)

    add_library(KTX::ktx ALIAS ktx)
endif()

# ----- Header-only libraries (interface targets) -----
# rapidjson
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/extern/rapidjson/include)

# stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/extern/stb)

# LuaBridge3
add_library(LuaBridge3 INTERFACE)
target_include_directories(LuaBridge3 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/extern/LuaBridge3/Source)

# ----- Create alias targets for compatibility -----
# These match the names expected by MagicEngine/CMakeLists.txt

# glm
if(NOT TARGET glm::glm-header-only AND TARGET glm::glm)
    add_library(glm::glm-header-only ALIAS glm::glm)
endif()

# Vulkan headers alias
if(NOT TARGET Vulkan::Headers AND TARGET Vulkan-Headers)
    add_library(Vulkan::Headers ALIAS Vulkan-Headers)
endif()

# VMA alias
if(NOT TARGET GPUOpen::VulkanMemoryAllocator AND TARGET VulkanMemoryAllocator)
    add_library(GPUOpen::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)
endif()

# volk aliases
if(NOT TARGET volk::volk_headers AND TARGET volk)
    add_library(volk::volk_headers ALIAS volk)
endif()

# spirv-cross aliases
if(NOT TARGET spirv-cross-core AND TARGET spirv-cross-core)
    # Already correct name
endif()

# SPIRV-Reflect alias
if(NOT TARGET unofficial::spirv-reflect AND TARGET spirv-reflect-static)
    add_library(unofficial::spirv-reflect ALIAS spirv-reflect-static)
endif()

# Freetype alias
if(NOT TARGET Freetype::Freetype AND TARGET freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()

# Jolt alias
if(NOT TARGET Jolt::Jolt AND TARGET Jolt)
    add_library(Jolt::Jolt ALIAS Jolt)
endif()

# Tracy alias (desktop only)
if(NOT ANDROID)
    if(NOT TARGET Tracy::TracyClient AND TARGET TracyClient)
        add_library(Tracy::TracyClient ALIAS TracyClient)
    endif()
endif()

# glslang aliases
if(NOT TARGET glslang::glslang AND TARGET glslang)
    add_library(glslang::glslang ALIAS glslang)
endif()
if(NOT TARGET glslang::SPIRV AND TARGET SPIRV)
    add_library(glslang::SPIRV ALIAS SPIRV)
endif()
if(NOT TARGET glslang::glslang-default-resource-limits AND TARGET glslang-default-resource-limits)
    add_library(glslang::glslang-default-resource-limits ALIAS glslang-default-resource-limits)
endif()

# Recast aliases
if(NOT TARGET RecastNavigation::Recast AND TARGET Recast)
    add_library(RecastNavigation::Recast ALIAS Recast)
endif()
if(NOT TARGET RecastNavigation::Detour AND TARGET Detour)
    add_library(RecastNavigation::Detour ALIAS Detour)
endif()
if(NOT TARGET RecastNavigation::DetourCrowd AND TARGET DetourCrowd)
    add_library(RecastNavigation::DetourCrowd ALIAS DetourCrowd)
endif()
if(NOT TARGET RecastNavigation::DetourTileCache AND TARGET DetourTileCache)
    add_library(RecastNavigation::DetourTileCache ALIAS DetourTileCache)
endif()
if(NOT TARGET RecastNavigation::DebugUtils AND TARGET DebugUtils)
    add_library(RecastNavigation::DebugUtils ALIAS DebugUtils)
endif()

# glfw alias (desktop only)
if(NOT ANDROID)
    if(NOT TARGET glfw3 AND TARGET glfw)
        add_library(glfw3 ALIAS glfw)
    endif()
endif()

# meshoptimizer alias (desktop only)
if(NOT ANDROID)
    if(NOT TARGET meshoptimizer::meshoptimizer AND TARGET meshoptimizer)
        add_library(meshoptimizer::meshoptimizer ALIAS meshoptimizer)
    endif()
endif()

message(STATUS "=== Submodule Dependencies Configured ===")
message(STATUS "")

# =============================================================================
# ANDROID SETTINGS
# =============================================================================

if(ANDROID)
    if(NOT ANDROID_NDK)
        message(FATAL_ERROR "ANDROID_NDK variable not set")
    endif()

    add_link_options(-Wl,-z,max-page-size=16384)

    set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    if(NOT EXISTS ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
        message(FATAL_ERROR "Android native app glue not found at: ${NATIVE_APP_GLUE_DIR}")
    endif()

    message(STATUS "Android configuration:")
    message(STATUS "  Platform: ${ANDROID_PLATFORM}")
    message(STATUS "  ABI: ${ANDROID_ABI}")
    message(STATUS "  NDK: ${ANDROID_NDK}")

    find_library(android-lib android)
    find_library(log-lib log)

    add_library(native_app_glue STATIC
        ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
    )
    target_include_directories(native_app_glue PUBLIC ${NATIVE_APP_GLUE_DIR})
    target_link_libraries(native_app_glue ${android-lib} ${log-lib})

    set(ANDROID_COMMON_LIBS
        native_app_glue
        ${android-lib}
        ${log-lib}
        CACHE INTERNAL "Common Android libraries"
    )
endif()

# =============================================================================
# HINA-VK INTEGRATION
# =============================================================================

# hina-vk expects certain target names
if(NOT TARGET glslang AND TARGET glslang::glslang)
    add_library(glslang ALIAS glslang::glslang)
endif()
if(NOT TARGET glslang-default-resource-limits AND TARGET glslang::glslang-default-resource-limits)
    add_library(glslang-default-resource-limits ALIAS glslang::glslang-default-resource-limits)
endif()
if(NOT ANDROID AND NOT TARGET TracyClient AND TARGET Tracy::TracyClient)
    add_library(TracyClient ALIAS Tracy::TracyClient)
endif()

set(HINA_VK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(HINA_VK_ENABLE_TRACY OFF CACHE BOOL "" FORCE)
set(HINA_VK_ENABLE_SHADER_COMPILER ON CACHE BOOL "" FORCE)

message(STATUS "Adding subdirectory: hina-vk")
add_subdirectory(extern/hina-vk)

# Provide include paths to hina-vk
target_include_directories(hina-vk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/volk
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/Vulkan-Headers/include
)

# =============================================================================
# ENGINE AND APPLICATIONS
# =============================================================================

add_subdirectory(MagicEngine)

if(NOT ANDROID)
    option(BUILD_ASSET_COMPILER "Build the asset compiler tool" ON)
    if(BUILD_ASSET_COMPILER)
        add_subdirectory(AssetCompiler)
    endif()

    add_subdirectory(editor)

    option(BUILD_GAME "Build the standalone game (no editor)" ON)
    if(BUILD_GAME)
        add_subdirectory(game)
    endif()
endif()

# =============================================================================
# POST CONFIGURATION
# =============================================================================

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
# Copy Directory.Build.props if it exists (desktop only)
if(EXISTS "${CMAKE_SOURCE_DIR}/Directory.Build.props")
    file(COPY "${CMAKE_SOURCE_DIR}/Directory.Build.props" DESTINATION "${CMAKE_BINARY_DIR}")
endif()
