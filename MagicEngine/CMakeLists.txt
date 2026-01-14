# MagicEngine/CMakeLists.txt
# Unified engine library - merges ryEngine-Core, ryEngine-ImGui, ryEngine-Tools, MagicEngine-Core

# Add offset_allocator sources as extra
file(GLOB_RECURSE OFFSET_ALLOCATOR_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/extern/include/offset_allocator/*.hpp"
    "${PROJECT_SOURCE_DIR}/extern/include/offset_allocator/*.cpp"
    "${PROJECT_SOURCE_DIR}/extern/include/offset_allocator/*.h"
)

# Glob all engine sources
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ipp"
)

# Create the unified static library
add_library(MagicEngine STATIC)
target_sources(MagicEngine PRIVATE
    ${ENGINE_SOURCES}
    ${OFFSET_ALLOCATOR_SOURCES}
)

# ----- Compile Definitions -----
set(ENGINE_DEFINITIONS
    VK_NO_PROTOTYPES=1
    QUILL_DISABLE_NON_PREFIXED_MACROS
    QUILL_NO_EXCEPTIONS
    IMGUI_ENABLE_FREETYPE
    $<$<CONFIG:Debug>:IMGUI_ENABLED>
)

if(NOT ANDROID)
    list(APPEND ENGINE_DEFINITIONS
        GLFW
        TRACY_ENABLE
    )
endif()

target_compile_definitions(MagicEngine PUBLIC ${ENGINE_DEFINITIONS})

# ----- Dependencies -----
# Public dependencies (exposed to consumers)
set(ENGINE_PUBLIC_DEPS
    glm::glm-header-only
    fmt::fmt
    quill::quill
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
    imguizmo::imguizmo
    Jolt::Jolt
    RecastNavigation::Recast
    RecastNavigation::Detour
    RecastNavigation::DetourCrowd
    RecastNavigation::DetourTileCache
    RecastNavigation::DebugUtils
    Boost::boost
    ${LUA_LIBRARIES}
)

# Private dependencies
set(ENGINE_PRIVATE_DEPS
    hina-vk
    volk::volk_headers
    spirv-cross-core
    spirv-cross-glsl
    unofficial::spirv-reflect
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
    Freetype::Freetype
)

# Desktop-only dependencies
if(NOT ANDROID)
    list(APPEND ENGINE_PRIVATE_DEPS
        assimp::assimp
        freeimage::FreeImage
        freeimage::FreeImagePlus
        KTX::ktx
        mikktspace::mikktspace
        meshoptimizer::meshoptimizer
        glslang::SPVRemapper
        Tracy::TracyClient
    )

    # GLOB libs locations for FMOD etc
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.lib")
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.lib")

    list(APPEND ENGINE_PUBLIC_DEPS
        $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
        $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
    )
else()
    # Android needs KTX linked differently
    list(APPEND ENGINE_PRIVATE_DEPS
        ktx
    )
    target_compile_definitions(MagicEngine PUBLIC KHRONOS_STATIC)
endif()

target_link_libraries(MagicEngine
    PUBLIC ${ENGINE_PUBLIC_DEPS}
    PRIVATE ${ENGINE_PRIVATE_DEPS}
)

# ----- Include Directories -----
target_include_directories(MagicEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/include>
        $<INSTALL_INTERFACE:${EXTERNAL_INCLUDES_INSTALL}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/include/offset_allocator>
        $<INSTALL_INTERFACE:${EXTERNAL_INCLUDES_INSTALL}/offset_allocator>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/hina-vk/include>
        $<INSTALL_INTERFACE:${EXTERNAL_INCLUDES_INSTALL}/hina-vk>
)

# Add all subdirectories as include paths (for legacy include compatibility)
file(GLOB ENGINE_SUBDIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(subdir ${ENGINE_SUBDIRS})
    if(IS_DIRECTORY ${subdir})
        target_include_directories(MagicEngine PUBLIC
            $<BUILD_INTERFACE:${subdir}>
        )
    endif()
endforeach()

# ----- Precompiled Header -----
target_precompile_headers(MagicEngine PRIVATE pch.h)

# im3d has its own Mat4 type that conflicts with MagicMath's global Mat4
# Exclude it from using the precompiled header
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/im3d.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON
)

# ----- Platform Configuration -----
engine_configure_platform(MagicEngine)

# ----- Compile Options -----
if(MSVC)
    target_compile_options(MagicEngine PRIVATE
        /Zc:preprocessor
        /bigobj
        /utf-8
    )
endif()

if(NOT ANDROID)
    # Debug renderer for Jolt
    target_compile_definitions(Jolt::Jolt INTERFACE JPH_DEBUG_RENDERER)

    # Check if a STATIC vcpkg triplet was used
    if(VCPKG_TARGET_TRIPLET MATCHES "-static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
        message(STATUS "Static vcpkg triplet detected. Forcing CMAKE_MSVC_RUNTIME_LIBRARY to /MT(d).")
    endif()
endif()

# Note: Installation handled by root CMakeLists.txt via install_config()

# Set folder for IDE
engine_set_folder(MagicEngine "Engine")
engine_set_source_groups(MagicEngine)

message(STATUS "MagicEngine unified library configured")
if(ANDROID)
    message(STATUS "  Platform: Android (ABI: ${ANDROID_ABI})")
else()
    message(STATUS "  Platform: Desktop")
endif()
