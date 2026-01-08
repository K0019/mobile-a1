# editor/CMakeLists.txt

# Create Editor executable (automatically globs all files in current directory)
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS 
    "*.cpp" 
    "*.h"
    "*.hpp"
)

# Include shaders for IDE organization
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
    "${PROJECT_SOURCE_DIR}/Shaders/*"
)

add_executable(Editor WIN32)

target_sources(Editor PRIVATE 
    ${EDITOR_SOURCES}
    ${SHADER_FILES}
)

# Link unified engine library
target_link_libraries(Editor PRIVATE
    MagicEngine
)

# Reuse MagicEngine's precompiled header
target_precompile_headers(Editor REUSE_FROM MagicEngine)
target_include_directories(Editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Organize shaders in IDE
source_group("Shaders" FILES ${SHADER_FILES})

# Platform-specific configuration
engine_configure_platform(Editor)

# MSVC-specific settings
if(MSVC)
    target_compile_options(Editor PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Zi>
		/Zc:preprocessor
    )
    
    # Visual Studio settings
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Editor)
    set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Editor>")
	
	set(PORTABLE_LAYERS_PATH "${CMAKE_SOURCE_DIR}/extern/vulkan_validation_layers")
        if(EXISTS "${PORTABLE_LAYERS_PATH}/VkLayer_khronos_validation.json")
            file(TO_NATIVE_PATH "${PORTABLE_LAYERS_PATH}" NATIVE_PORTABLE_LAYERS_PATH)
            message(STATUS "Configuring portable Vulkan layers: ${NATIVE_PORTABLE_LAYERS_PATH}")
            set_property(TARGET Editor PROPERTY VS_DEBUGGER_ENVIRONMENT
                "VK_LAYER_PATH=${NATIVE_PORTABLE_LAYERS_PATH}\nVK_LOADER_DEBUG=all"
            )
        endif()
endif()

if(NOT ANDROID)
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.dll")
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.dll")
    set(EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES "${EXTERNAL_LIBRARIES}/directory_copy")
    # To reformat into a style that copy_if_different can accept
    string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}")
    string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}")

    add_custom_command(TARGET Editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "\"$<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}>$<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}>\""
            "\"$<TARGET_FILE_DIR:Editor>\""
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "\"${EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES}\""
            "\"$<TARGET_FILE_DIR:Editor>\""
        COMMAND cmd.exe /c if "$(Configuration)" == "Release" "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:Editor>/Assets"
    )
endif()

# --- Setting up Vulkan validation layers ---
if(WIN32 AND NOT ANDROID)
	set(PORTABLE_LAYERS_PATH "${CMAKE_SOURCE_DIR}/extern/vulkan-debug-layer")

        if(EXISTS "${PORTABLE_LAYERS_PATH}/VkLayer_khronos_validation.json")

            file(TO_NATIVE_PATH "${PORTABLE_LAYERS_PATH}" NATIVE_PORTABLE_LAYERS_PATH)

            message(STATUS "Configuring portable Vulkan layers: ${NATIVE_PORTABLE_LAYERS_PATH}")

            set_property(TARGET Editor PROPERTY VS_DEBUGGER_ENVIRONMENT

                "VK_LAYER_PATH=${NATIVE_PORTABLE_LAYERS_PATH}\nVK_LOADER_DEBUG=all"

            )

endif()
endif()

engine_set_folder(Editor "Editor")
engine_set_source_groups(Editor)
