# ryEngine-core/CMakeLists.txt

# Add offset_allocator sources as extra
file(GLOB_RECURSE OFFSET_ALLOCATOR_SOURCES CONFIGURE_DEPENDS 
    "${PROJECT_SOURCE_DIR}/extern/offset_allocator/*.hpp"
    "${PROJECT_SOURCE_DIR}/extern/offset_allocator/*.cpp"
    "${PROJECT_SOURCE_DIR}/extern/offset_allocator/*.h"
)

set(CORE_DEFINITIONS
    VK_NO_PROTOTYPES=1
    QUILL_DISABLE_NON_PREFIXED_MACROS
    QUILL_NO_EXCEPTIONS
)

# Platform-specific dependencies
set(CORE_PUBLIC_DEPS
    glm::glm-header-only
    fmt::fmt
    quill::quill
    GPUOpen::VulkanMemoryAllocator
)

set(CORE_PRIVATE_DEPS
    volk::volk_headers
    spirv-cross-core
    spirv-cross-glsl
    unofficial::spirv-reflect
	glslang::glslang          # Add this
    glslang::SPIRV           # And this  
    glslang::glslang-default-resource-limits  # And this if needed
)

# Add desktop-only dependencies
if(NOT ANDROID)
    list(APPEND CORE_PRIVATE_DEPS
        # Add any desktop-specific dependencies here if needed
    )
	list(APPEND CORE_DEFINITIONS
		TRACY_ENABLE
		)
endif()

# Create core engine library (globs current directory + extra sources)
engine_add_module(ryEngine-Core
    FOLDER "ryEngine-core"
    EXTRA_SOURCES ${OFFSET_ALLOCATOR_SOURCES}
    PUBLIC_DEPENDENCIES ${CORE_PUBLIC_DEPS}
    DEPENDENCIES ${CORE_PRIVATE_DEPS}
    DEFINITIONS ${CORE_DEFINITIONS}
)

if(MSVC)
    target_compile_options(ryEngine-Core PRIVATE /Zc:preprocessor)
endif()

# Include directories
target_include_directories(ryEngine-Core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/extern/offset_allocator>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/offset_allocator>
)

# Platform-specific configuration (handles Android native app glue automatically)
engine_configure_platform(ryEngine-Core)

message(STATUS "Core CMake - CMAKE_INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "Core CMake - CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

install(TARGETS ryEngine-Core
    EXPORT MagicEngineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ryEngine
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
    PATTERN "*.ipp"
)

# Install offset_allocator headers
install(DIRECTORY ${PROJECT_SOURCE_DIR}/extern/offset_allocator/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/offset_allocator
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp"
)

# Additional Android-specific setup if needed
if(ANDROID)
    # Any additional Android-specific configuration can go here
    message(STATUS "ryEngine-Core configured for Android (ABI: ${ANDROID_ABI})")
else()
    message(STATUS "ryEngine-Core configured for desktop")
endif()