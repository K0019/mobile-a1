cmake_minimum_required(VERSION 3.26)

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

# ----- External Libraries Configuration ------

set(EXTERNAL_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/extern/include")
set(EXTERNAL_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/extern/lib")

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

find_package(assimp CONFIG REQUIRED)
find_package(FreeImage CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(KTX CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
find_package(quill CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tracy CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(Volk CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
add_subdirectory(extern/offset_allocator)

# ----- Engine Target -----
add_executable(Engine WIN32) #

# Search for files needed
file(GLOB_RECURSE ENGINE_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "engine/*.cpp")
file(GLOB_RECURSE ENGINE_HEADER_FILES CONFIGURE_DEPENDS "engine/*.h" "engine/*.hpp" "engine/*.ipp")
file(GLOB_RECURSE SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.vert" "${CMAKE_SOURCE_DIR}/shaders/*.frag")
file(GLOB_RECURSE ENGINE_SOURCE_FILES_DIRECTORIES LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/engine/*")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.dll")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.dll")
set(EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES "${EXTERNAL_LIBRARIES}/directory_copy")
# To reformat into a style that copy_if_different can accept
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}")
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}")
# To reformat for xcopy
string(REPLACE "/" "\\" COMPILED_SHADER_FILES "${CMAKE_SOURCE_DIR}/Shaders/*.spv")
string(REPLACE "/" "\\" OUTPUT_COMPILED_SHADER_FILES "$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Assets/Shaders>$<$<CONFIG:Release>:$<TARGET_FILE_DIR:Engine>/Assets/Shaders>")

target_sources(Engine PRIVATE
    ${ENGINE_IMPLEMENTATION_FILES}
    ${ENGINE_HEADER_FILES}
    ${SHADER_FILES}
)

target_precompile_headers(Engine PRIVATE engine/pch.h)

target_include_directories(Engine PUBLIC
    ${CMAKE_SOURCE_DIR}/engine
    ${EXTERNAL_INCLUDES}
)
foreach(item ${ENGINE_SOURCE_FILES_DIRECTORIES})
	if(IS_DIRECTORY ${item})
		target_include_directories(Engine PUBLIC ${item})
	endif()
endforeach()

target_link_libraries(Engine PUBLIC
    # --- Core Dependencies (from vcpkg) ---
    # Correct namespaced targets are used as per vcpkg output.
    assimp::assimp
    freeimage::FreeImage 
	freeimage::FreeImagePlus # FreeImage::FreeImagePlus is also available if needed
    Freetype::Freetype
    glfw
    glm::glm-header-only
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
    KTX::ktx
    meshoptimizer::meshoptimizer
	mikktspace::mikktspace
    quill::quill
    Tracy::TracyClient
    volk::volk_headers  # This provides the Vulkan headers
    Vulkan::Vulkan
	Jolt::Jolt
    # Shader compilation
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
    glslang::SPVRemapper
    spirv-cross-core
    spirv-cross-glsl
    unofficial::spirv-reflect
	offsetAllocator
    # External libraries
    $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
    $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
)

set_source_groups(Engine)

target_compile_options(Engine PRIVATE /W4 /MP /bigobj)
target_compile_definitions(Engine PUBLIC
    TRACY_ENABLE
	GLFW
    -DQUILL_DISABLE_NON_PREFIXED_MACROS
    $<$<CONFIG:Debug>:IMGUI_ENABLED>
)

add_custom_command(TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "\"$<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}>$<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}>\""
        "\"$<TARGET_FILE_DIR:Engine>\""
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "\"${EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES}\""
        "\"$<TARGET_FILE_DIR:Engine>\""
    COMMAND cmd.exe /c if "$(Configuration)" == "Release" "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:Engine>/Assets"
    COMMAND cmd.exe /c "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Shaders" "$<TARGET_FILE_DIR:Engine>/Shaders"
    
)

if(MSVC)
    target_compile_options(Engine PUBLIC
        /wd4819 # Unicode characters in non-unicode formatted source files
        /wd4324 # Structure padding added due to explicit alignas() specifier
    )
    target_compile_definitions(Engine PUBLIC
        WIN32_LEAN_AND_MEAN
    )
    target_link_options(Engine PUBLIC
        $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
    )
    set_target_properties(Engine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Engine>"
    )
	set(PORTABLE_LAYERS_PATH "${CMAKE_SOURCE_DIR}/extern/vulkan-debug-layer")
	if(EXISTS "${PORTABLE_LAYERS_PATH}/VkLayer_khronos_validation.json")
		file(TO_NATIVE_PATH "${PORTABLE_LAYERS_PATH}" NATIVE_PORTABLE_LAYERS_PATH)
		message(STATUS "Configuring portable Vulkan layers for MSVC debugging at: ${NATIVE_PORTABLE_LAYERS_PATH}")

		set_property(TARGET Engine PROPERTY
			VS_DEBUGGER_ENVIRONMENT
				"VK_LAYER_PATH=${NATIVE_PORTABLE_LAYERS_PATH}
				 VK_LOADER_DEBUG=all"
		)
	else()
		message(WARNING "Portable Vulkan layer manifest not found at ${PORTABLE_LAYERS_PATH}")
	endif()
endif()


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


# ----- Target Dependencies -----

add_dependencies(Engine EngineScripting)

# ----- Global Build Settings -----
# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Engine)

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
        $<$<CONFIG:Release>:/O2 /GL> # Optimization level for Release
        $<$<CONFIG:Debug>:/Zi> # Or /Zi for debug info format
    )
	string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /ignore:4099 /ignore:4098")
    add_link_options(
         $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
endif()

# Default CMake configuration sets the intermediate output directory of all projects to the same location.
# This causes issues with the C++ project trying to reference C# Nuget stuff when it doesn't need to.
# This file overrides the intermediate output directory location to avoid this issue.
file(COPY "${CMAKE_SOURCE_DIR}/Directory.Build.props" DESTINATION "${CMAKE_BINARY_DIR}")