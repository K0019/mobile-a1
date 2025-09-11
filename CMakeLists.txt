cmake_minimum_required(VERSION 3.21)

# ----- Function to organize source files into logical folders in IDEs -----
function(set_source_groups target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    foreach(source ${target_sources})
        get_filename_component(source_path "${source}" PATH)
        # Ensure the path is relative to the source directory for cleaner filter names
        file(RELATIVE_PATH rel_source_path ${CMAKE_SOURCE_DIR} ${source_path})
        if(rel_source_path)
            # Use forward slashes for CMake source_group
            string(REPLACE "\\" "/" source_group_path "${rel_source_path}")
            source_group("${source_group_path}" FILES "${source}")
        else()
             # Handle files directly in CMAKE_SOURCE_DIR if any (less common for targets)
             # source_group("Source Files" FILES "${source}") # Example: Source files directly in root
        endif()
    endforeach()
endfunction()


# ----- VCPKG Configuration -----

set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg")
# If the user hasn't specified a toolchain file, set it to our bundled vcpkg.
# This makes configuration "just work" without extra command-line flags.
if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file" FORCE)
    message(STATUS "Forcing use of bundled vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
else()
    message(WARNING "vcpkg submodule not found at ${VCPKG_ROOT}. Dependency management may fail. Did you clone with --recurse-submodules?")
endif()

# Check if vcpkg has been bootstrapped. If not, run the bootstrap script automatically.
if(EXISTS "${VCPKG_ROOT}" AND NOT EXISTS "${VCPKG_ROOT}/vcpkg.exe")
    message(STATUS "vcpkg not bootstrapped yet. Running bootstrap script...")
    if(WIN32)
        execute_process(COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.bat" "-disableMetrics" RESULT_VARIABLE VCPKG_BOOTSTRAP_RESULT)
    else()
        execute_process(COMMAND "${VCPKG_ROOT}/bootstrap-vcpkg.sh" "-disableMetrics" RESULT_VARIABLE VCPKG_BOOTSTRAP_RESULT)
    endif()

    if(NOT VCPKG_BOOTSTRAP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to bootstrap vcpkg. Please try running the bootstrap script manually.")
    endif()
    message(STATUS "vcpkg bootstrapped successfully.")
endif()

# ----- Solution -----

project(MagicEngine VERSION 0.1.0 LANGUAGES C CXX CSharp)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories for binaries and libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Disable Assimp command-line tools")
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "Disable Assimp samples")
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable model exporting to reduce library size")
set(ASSIMP_INSTALL OFF CACHE BOOL "Disable install targets for Assimp")
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Do not treat warnings as errors for third-party libs")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "Optional: Disable Draco if not using compressed glTF")

find_package(assimp CONFIG REQUIRED)
find_package(entt CONFIG REQUIRED)
find_package(FreeImage CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED) 
find_package(KTX CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)
find_package(tracy CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Volk CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
find_package(enkiTS CONFIG REQUIRED)
#add_subdirectory(extern/offset_allocator)
find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue/atomicops.h")

# ----- Engine Target -----
add_executable(Engine WIN32) #

# Add engine's own source/header files
file(GLOB_RECURSE ENGINE_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADER_FILES CONFIGURE_DEPENDS "engine/*.h" "engine/*.hpp" "engine/*.ipp")
file(GLOB_RECURSE IMGUI_HEADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/engine/editor/imgui/src/*.h" #
    "${CMAKE_SOURCE_DIR}/engine/editor/imgui/impl/*.h" # [cite: 382]
    "${CMAKE_SOURCE_DIR}/engine/editor/imgui/src/ImGuizmo/*.h" #
	"${CMAKE_SOURCE_DIR}/engine/editor/imgui/src/ImPlot/*.h" #
)
file(GLOB TRACY_MANUAL_SOURCES CONFIGURE_DEPENDS "extern/tracy/TracyClient.cpp") # Tracy vcpkg port is header-only
file(GLOB_RECURSE SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*")

target_sources(Engine PRIVATE
    ${ENGINE_IMPLEMENTATION_FILES}
    ${ENGINE_HEADER_FILES}
    ${IMGUI_IMPLEMENTATION_FILES}
    ${IMGUI_HEADER_FILES} # Add ImGui headers
    ${TRACY_MANUAL_SOURCES} # Tracy needs its client cpp file compiled in
    ${SHADER_FILES}
)

target_precompile_headers(Engine PRIVATE engine/pch.h)

target_include_directories(Engine PUBLIC
    ${CMAKE_SOURCE_DIR}/engine
    ${CMAKE_SOURCE_DIR}/engine/editor/imgui/src #
    ${CMAKE_SOURCE_DIR}/engine/editor/imgui/impl #
    ${READERWRITERQUEUE_INCLUDE_DIRS} # Add include dir for header-only lib
)

target_link_libraries(Engine PUBLIC
    # --- Core Dependencies (from vcpkg) ---
    # Correct namespaced targets are used as per vcpkg output.
    assimp::assimp
    EnTT::EnTT
	enkiTS::enkiTS
    freeimage::FreeImage 
	freeimage::FreeImagePlus # FreeImage::FreeImagePlus is also available if needed
    Freetype::Freetype
    glfw
    glm::glm-header-only
    KTX::ktx
    meshoptimizer::meshoptimizer
    quill::quill
    Tracy::TracyClient
    volk::volk_headers  # This provides the Vulkan headers
    GPUOpen::VulkanMemoryAllocator
	mikktspace::mikktspace
    # Shader compilation
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
    glslang::SPVRemapper
    spirv-cross-core
    spirv-cross-glsl
    unofficial::spirv-reflect
	offsetAllocator
    $<$<TARGET_EXISTS:mimalloc-static>:mimalloc-static;mimalloc>
)

target_compile_options(Engine PRIVATE /W4 /MP /bigobj)

target_compile_definitions(Engine PUBLIC
    TRACY_ENABLE
    -DQUILL_DISABLE_NON_PREFIXED_MACROS
)
if(MSVC)
    target_compile_definitions(Engine PUBLIC
        WIN32_LEAN_AND_MEAN
    )
endif()

# Apply source groups for Engine
set_source_groups(Engine)


# ----- C# Target -----

add_library(EngineScripting SHARED)

file(GLOB_RECURSE CSHARP_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "EngineScripting/*.cs")

target_sources(EngineScripting PRIVATE
    ${CSHARP_IMPLEMENTATION_FILES}
)

set_source_groups(EngineScripting)

set_property(TARGET EngineScripting PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")
set_property(TARGET EngineScripting PROPERTY VS_DOTNET_REFERENCES
    "Microsoft.CSharp"
    "System"
    "System.Core"
    "System.Xml.Linq"
    "System.Data.DataSetExtensions"
    "System.Data"
    "System.Net.Http"
    "System.Xml"
)
set_property(TARGET EngineScripting PROPERTY VS_PACKAGE_REFERENCES "GlmSharp_0.9.8")

# ----- Global Build Settings -----
# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Engine)
set_property(TARGET Engine PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_definitions("-DVK_NO_PROTOTYPES=1")
if(WIN32)
  add_definitions("-DVK_USE_PLATFORM_WIN32_KHR=1")
  add_definitions("-DNOMINMAX")
endif()

if(DEFINED ENV{VULKAN_SDK})
    set(VK_LAYER_PATH "$ENV{VULKAN_SDK}/Bin")
endif()

if(MSVC)
    add_compile_options(
        /W4 # Warning level 4
        /MP # Enable parallel builds
        $<$<CONFIG:Release>:/O2 /GL> # Optimization level for Release
        $<$<CONFIG:Debug>:/Zi> # Or /Zi for debug info format
    )
	string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    # add_compile_options(/wd4201) # Example: nonstandard extension used : nameless struct/union
    add_link_options(
         $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()
