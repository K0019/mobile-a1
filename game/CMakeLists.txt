# game/CMakeLists.txt
# Standalone game executable (no editor)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    "*.hpp"
)

add_executable(Game WIN32)

target_sources(Game PRIVATE
    ${GAME_SOURCES}
)

# Link unified engine library
target_link_libraries(Game PRIVATE
    MagicEngine
)

# Use MagicEngine's pch.h but compile our own PCH (avoid definition mismatch)
target_precompile_headers(Game PRIVATE ${CMAKE_SOURCE_DIR}/MagicEngine/pch.h)
target_include_directories(Game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Platform-specific configuration
engine_configure_platform(Game)

# MSVC-specific settings
if(MSVC)
    target_compile_options(Game PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi>
        /Zc:preprocessor
    )

    set_property(TARGET Game PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Game>")
endif()

# Copy DLLs
if(NOT ANDROID)
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.dll")
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.dll")
    set(EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES "${EXTERNAL_LIBRARIES}/directory_copy")

    string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}")
    string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}")

    add_custom_command(TARGET Game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "\"$<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}>$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}>\""
            "\"$<TARGET_FILE_DIR:Game>\""
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "\"${EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES}\""
            "\"$<TARGET_FILE_DIR:Game>\""
        # Always copy assets for game builds
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "\"${CMAKE_SOURCE_DIR}/Assets\""
            "\"$<TARGET_FILE_DIR:Game>/Assets\""
    )
endif()

engine_set_folder(Game "Game")
engine_set_source_groups(Game)

message(STATUS "Game executable configured (no editor)")
