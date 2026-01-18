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

target_sources(AssetCompiler PRIVATE
    ${ASSET_COMPILER_IMPLEMENTATION_FILES}
    ${ASSET_COMPILER_HEADER_FILES}
)

#target_precompile_headers(AssetCompiler PRIVATE asset_compiler/pch.h)

target_include_directories(AssetCompiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/AssetCompiler>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/engine>
    $<BUILD_INTERFACE:${EXTERNAL_INCLUDES}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(AssetCompiler PRIVATE
    glm::glm-header-only
    Vulkan::Headers
    assimp::assimp
    meshoptimizer::meshoptimizer
    KTX::ktx
	mikktspace::mikktspace
    compressonator
)

install(TARGETS AssetCompiler 
    RUNTIME DESTINATION bin
)

set_target_properties(AssetCompiler PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Tools"
)

engine_set_source_groups(AssetCompiler)
