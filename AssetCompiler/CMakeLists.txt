#  ----- Compressonator imported target -----
add_library(compressonator STATIC IMPORTED)
set_target_properties(compressonator PROPERTIES
    IMPORTED_LOCATION_DEBUG   "${CMAKE_SOURCE_DIR}/extern/lib/compressonator/Compressonator_MDd.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/extern/lib/compressonator/Compressonator_MD.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/extern/include/compressonator"
)

# ----- AssetCompiler Target -----
#add_library(AssetCompiler)
# Oh no!!! school pc cannot work with compressonator for some reason!!! make the compiler an .exe instead!
add_executable(AssetCompiler)

file(GLOB_RECURSE ASSET_COMPILER_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ASSET_COMPILER_HEADER_FILES CONFIGURE_DEPENDS "*.h")

# ufbx - single-file FBX loader (C source)
set(UFBX_SOURCE "${CMAKE_SOURCE_DIR}/extern/include/ufbx/ufbx.c")

target_sources(AssetCompiler PRIVATE
    ${ASSET_COMPILER_IMPLEMENTATION_FILES}
    ${ASSET_COMPILER_HEADER_FILES}
    ${UFBX_SOURCE}
)

# ufbx include directory
target_include_directories(AssetCompiler PRIVATE
    ${CMAKE_SOURCE_DIR}/extern/include/ufbx
)

#target_precompile_headers(AssetCompiler PRIVATE asset_compiler/pch.h)

target_include_directories(AssetCompiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/AssetCompiler>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/MagicEngine>
    $<BUILD_INTERFACE:${EXTERNAL_INCLUDES}>
    $<INSTALL_INTERFACE:include>
)

# Link MagicEngine - provides assimp, meshoptimizer, ktx, mikktspace, glm, and all asset processing code
# Compressonator is compiler-specific (texture compression for BC formats)
# astcenc-avx2-static provides ASTC compression (ARM's official encoder)
# Vulkan::Headers needed for VkFormat enums in texture compilation
target_link_libraries(AssetCompiler PRIVATE
    MagicEngine
    Vulkan::Headers
    compressonator
    astcenc-avx2-static
)

install(TARGETS AssetCompiler 
    RUNTIME DESTINATION bin
)

set_target_properties(AssetCompiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Tools"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Tools"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Tools"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/Tools"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/Tools"
)

engine_set_source_groups(AssetCompiler)
