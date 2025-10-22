# ----- Engine Target -----
# Preprocessor definitions
if(NOT ANDROID)
    set(CORE_DEFINITIONS
        $<$<CONFIG:Debug>:IMGUI_ENABLED>
    )
	list(APPEND CORE_DEFINITIONS
		GLFW
    )
endif()

# Platform-specific dependencies
if (NOT ANDROID)
    # GLOB libs locations
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.lib")
    file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.lib")

    set(CORE_PUBLIC_DEPS
        $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
        $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
        glm::glm-header-only
        imgui::imgui
        imguizmo::imguizmo
        Jolt::Jolt
        KTX::ktx
    )
else()
    set(CORE_PUBLIC_DEPS
        $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
        $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
        glm::glm-header-only
        Jolt::Jolt
    )
endif()

# Create the target
engine_add_module(MagicEngine-Core
    FOLDER "MagicEngine-core"
    DEPENDENCIES ${CORE_PRIVATE_DEPS}
    PUBLIC_DEPENDENCIES ${CORE_PUBLIC_DEPS}
    DEFINITIONS ${CORE_DEFINITIONS}
)

target_link_libraries(MagicEngine-Core PRIVATE 
    $<$<NOT:$<BOOL:${ANDROID}>>:KTX::ktx>
    $<$<BOOL:${ANDROID}>:ktx>
)

if(ANDROID)
    target_compile_definitions(MagicEngine-Core PUBLIC KHRONOS_STATIC)
endif()

# link asset compiler to engine for resource building
if(NOT ANDROID)
    target_link_libraries(MagicEngine-Core PRIVATE AssetCompiler EngineScripting)
endif()

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
install(DIRECTORY ${PROJECT_SOURCE_DIR}/extern/include/offset_allocator/
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
        $<BUILD_INTERFACE:${EXTERNAL_INCLUDES}/offset_allocator> # required due to ryEngine externals set up as such
        $<INSTALL_INTERFACE:${EXTERNAL_INCLUDES_INSTALL}/offset_allocator>
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
if (NOT ANDROID)
    target_compile_options(MagicEngine-Core PRIVATE /bigobj)
endif()
