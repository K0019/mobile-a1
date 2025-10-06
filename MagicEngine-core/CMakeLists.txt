# ----- Engine Target -----

# File locations
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.dll")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.dll")
set(EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES "${EXTERNAL_LIBRARIES}/directory_copy")
# To reformat into a style that copy_if_different can accept
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}")
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}")


# Preprocessor definitions
set(CORE_DEFINITIONS
    $<$<CONFIG:Debug>:IMGUI_ENABLED>
)
if(NOT ANDROID)
	list(APPEND CORE_DEFINITIONS
		GLFW
    )
endif()

# Platform-specific dependencies
set(CORE_PUBLIC_DEPS
    $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
    $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
)
# set(CORE_PRIVATE_DEPS
# )

# Create the target
engine_add_module(MagicEngine-Core
    FOLDER "MagicEngine-core"
    DEPENDENCIES ${CORE_PRIVATE_DEPS}
    PUBLIC_DEPENDENCIES ${CORE_PUBLIC_DEPS}
    DEFINITIONS ${CORE_DEFINITIONS}
)

# Install for later linking to the whole solution
engine_configure_platform(MagicEngine-Core)
install(TARGETS MagicEngine-Core
    EXPORT MagicEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MagicEngine
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
    PATTERN "*.ipp"
)
# Install external headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/extern/offset_allocator/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/offset_allocator
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Set include headers
target_precompile_headers(MagicEngine-Core PRIVATE pch.h)
target_include_directories(MagicEngine-Core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${EXTERNAL_INCLUDES}>
        $<INSTALL_INTERFACE:${EXTERNAL_INCLUDES_INSTALL}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ryEngine-core>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ryEngine-modules>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ryEngine>
)
file(GLOB_RECURSE ENGINE_SOURCE_FILES_DIRECTORIES LIST_DIRECTORIES true $<BUILD_INTERFACE:"*">$<INSTALL_INTERFACE:"${CMAKE_INSTALL_INCLUDEDIR}/MagicEngine/*">)
foreach(item ${ENGINE_SOURCE_FILES_DIRECTORIES})
	if(IS_DIRECTORY ${item})
		target_include_directories(MagicEngine-Core PUBLIC ${item})
	endif()
endforeach()

# Additional compile options
target_compile_options(MagicEngine-Core PRIVATE /bigobj)

add_custom_command(TARGET MagicEngine-Core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "\"$<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}>$<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}>\""
        "\"$<TARGET_FILE_DIR:MagicEngine-Core>\""
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "\"${EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES}\""
        "\"$<TARGET_FILE_DIR:MagicEngine-Core>\""
    COMMAND cmd.exe /c if "$(Configuration)" == "Release" "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:MagicEngine-Core>/Assets"
    #COMMAND cmd.exe /c "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Shaders" "$<TARGET_FILE_DIR:Engine>/Shaders"
)
