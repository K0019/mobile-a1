#  ----- Compressonator imported target -----
add_library(compressonator STATIC IMPORTED)
set_target_properties(compressonator PROPERTIES
    IMPORTED_LOCATION_DEBUG   "${CMAKE_SOURCE_DIR}/extern/lib/compressonator/Compressonator_MDd.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/extern/lib/compressonator/Compressonator_MD.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/extern/include/compressonator"
)

# ----- AssetCompiler Target -----
add_library(AssetCompiler)

file(GLOB_RECURSE ASSET_COMPILER_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ASSET_COMPILER_HEADER_FILES CONFIGURE_DEPENDS "*.h")

target_sources(AssetCompiler PRIVATE
    ${ASSET_COMPILER_IMPLEMENTATION_FILES}
    ${ASSET_COMPILER_HEADER_FILES}
)

#target_precompile_headers(AssetCompiler PRIVATE asset_compiler/pch.h)

target_include_directories(AssetCompiler PUBLIC
    ${CMAKE_SOURCE_DIR}/AssetCompiler
    ${CMAKE_SOURCE_DIR}/engine
    ${EXTERNAL_INCLUDES}
)

target_link_libraries(AssetCompiler PRIVATE
    assimp::assimp
    meshoptimizer::meshoptimizer
    KTX::ktx
	mikktspace::mikktspace
    compressonator
)

engine_set_source_groups(AssetCompiler)
