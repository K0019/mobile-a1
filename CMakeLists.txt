cmake_minimum_required(VERSION 3.26)

include(cmake/EngineUtils.cmake)

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

set(VCPKG_MANIFEST_MODE ON)
set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT ANDROID)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

# ----- External Libraries Configuration ------

set(EXTERNAL_INCLUDES "${CMAKE_SOURCE_DIR}/extern/include")
set(EXTERNAL_LIBRARIES "${CMAKE_SOURCE_DIR}/extern/lib")

# ----- Solution -----

project(MagicEngine VERSION 0.1.0 LANGUAGES C CXX CSharp)
# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_DIRECTORIES_BEFORE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS OFF) # Disable putting projects into folders in the solution directory

# Output directories for binaries and libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Filename postfix for libraries under DEBUG configuration")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Core dependencies (all platforms)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(Volk CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(VulkanUtilityLibraries CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)

find_package(Jolt CONFIG REQUIRED)
find_package(Stb REQUIRED)

# ImGui dependencies (cross-platform)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

# Desktop-only dependencies
if(NOT ANDROID)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(tracy CONFIG REQUIRED)
    
    # Tools dependencies
    find_package(assimp CONFIG REQUIRED)
    find_package(FreeImage CONFIG REQUIRED)
    find_package(KTX CONFIG REQUIRED)
    find_package(meshoptimizer CONFIG REQUIRED)
    find_package(mikktspace CONFIG REQUIRED)
endif()

# ----- Android Settings -----
if(ANDROID)
    # Validate NDK path
    if(NOT ANDROID_NDK)
        message(FATAL_ERROR "ANDROID_NDK variable not set")
    endif()
    
    add_link_options(-Wl,-z,max-page-size=16384)
    # Check for native app glue availability
    set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    if(NOT EXISTS ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c)
        message(FATAL_ERROR "Android native app glue not found at: ${NATIVE_APP_GLUE_DIR}")
    endif()
    
    message(STATUS "Android configuration:")
    message(STATUS "  Platform: ${ANDROID_PLATFORM}")
    message(STATUS "  ABI: ${ANDROID_ABI}")
    message(STATUS "  NDK: ${ANDROID_NDK}")
    message(STATUS "  Native app glue: ${NATIVE_APP_GLUE_DIR}")

    # Find required Android libraries
    find_library(android-lib android)
    find_library(log-lib log)
    
    # Add native app glue
    set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    add_library(native_app_glue STATIC
        ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
    )
    target_include_directories(native_app_glue PUBLIC
        ${NATIVE_APP_GLUE_DIR}
    )
    target_link_libraries(native_app_glue ${android-lib} ${log-lib})
    
    # Make native_app_glue available to all targets
    set(ANDROID_COMMON_LIBS 
        native_app_glue
        ${android-lib}
        ${log-lib}
        CACHE INTERNAL "Common Android libraries"
    )
    
    message(STATUS "Android native app glue configured")
endif()

# ----- Global project settings -----

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
        /W4 # Warning level 4
        /MP # Enable parallel builds
        # $<$<CONFIG:Release>:/O2 /GL> # Optimization level for Release
        # $<$<CONFIG:Debug>:/Zi> # Or /Zi for debug info format
        /wd4819 # Unicode characters in non-unicode formatted source files
        /wd4324 # Structure padding added due to explicit alignas() specifier
    )
	string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    # add_link_options(
    #      $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    # )
endif()

message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
if(ANDROID)
    message(STATUS "Android ABI: ${ANDROID_ABI}")
endif()

# ----- Subdirectories (Add Projects) -----
# add_subdirectory(AssetCompiler)
# add_subdirectory(EngineScripting)
# add_subdirectory(extern/offset_allocator)
# add_subdirectory(engine)

set(namespace "ryengine")
add_subdirectory(ryEngine-core)
add_subdirectory(ryEngine-modules/imgui)
if(NOT ANDROID)
    add_subdirectory(ryEngine-modules/tools)
    add_subdirectory(editor)
endif()

add_library(MagicEngine INTERFACE)
target_link_libraries(MagicEngine INTERFACE 
    ryEngine-Core
    ryEngine-ImGui
)

# Create alias for export
add_library(ryengine::ryEngine ALIAS MagicEngine)

# ----- Target Dependencies -----
# add_dependencies(Engine AssetCompiler)
# add_dependencies(Engine EngineScripting)

# ----- Post Configuration Settings -----
# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)

# Default CMake configuration sets the intermediate output directory of all projects to the same location.
# This causes issues with the C++ project trying to reference C# Nuget stuff when it doesn't need to.
# This file overrides the intermediate output directory location to avoid this issue.
file(COPY "${CMAKE_SOURCE_DIR}/Directory.Build.props" DESTINATION "${CMAKE_BINARY_DIR}")

# Installation
include(cmake/InstallConfig.cmake)
