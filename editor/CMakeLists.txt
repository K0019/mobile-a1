# editor/CMakeLists.txt

# Create Editor executable (automatically globs all files in current directory)
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS 
    "*.cpp" 
    "*.h"
    "*.hpp"
)

# Include shaders for IDE organization
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS 
    "${PROJECT_SOURCE_DIR}/shaders/*"
)

add_executable(Editor WIN32)

target_sources(Editor PRIVATE 
    ${EDITOR_SOURCES}
    ${SHADER_FILES}
)

# Link everything
target_link_libraries(Editor PRIVATE
    ryEngine-Core
    ryEngine-ImGui
    ryEngine-Tools
)

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
    set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	
	set(PORTABLE_LAYERS_PATH "${CMAKE_SOURCE_DIR}/extern/vulkan_validation_layers")
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
