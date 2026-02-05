cmake_minimum_required(VERSION 3.22)
project("MagicEngineAndroid")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android page size alignment
add_link_options(-Wl,-z,max-page-size=16384)

message(STATUS "=== Android MagicEngine Build (Unified) ===")
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Android NDK: ${ANDROID_NDK}")

# Get the project root (from android/app/src/main/cpp/ up 5 levels)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
message(STATUS "Project root: ${PROJECT_ROOT}")

# ----- FMOD (Android-specific, not built from source) -----
set(FMOD_ROOT "${PROJECT_ROOT}/extern/lib/fmodAndroid")
set(FMOD_CORE_LOCATION "${FMOD_ROOT}/${ANDROID_ABI}/libfmod.so")
set(FMOD_STUDIO_LOCATION "${FMOD_ROOT}/${ANDROID_ABI}/libfmodstudio.so")

if(NOT EXISTS "${FMOD_CORE_LOCATION}")
    message(FATAL_ERROR "FMOD core not found at ${FMOD_CORE_LOCATION}")
endif()
if(NOT EXISTS "${FMOD_STUDIO_LOCATION}")
    message(FATAL_ERROR "FMOD studio not found at ${FMOD_STUDIO_LOCATION}")
endif()
message(STATUS "FMOD core found at ${FMOD_CORE_LOCATION}")
message(STATUS "FMOD studio found at ${FMOD_STUDIO_LOCATION}")

add_library(fmod_core SHARED IMPORTED)
set_target_properties(fmod_core PROPERTIES
    IMPORTED_LOCATION "${FMOD_CORE_LOCATION}"
    IMPORTED_NO_SONAME TRUE
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_ROOT}/extern/include"
)

add_library(fmod_studio SHARED IMPORTED)
set_target_properties(fmod_studio PROPERTIES
    IMPORTED_LOCATION "${FMOD_STUDIO_LOCATION}"
    IMPORTED_NO_SONAME TRUE
    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_ROOT}/extern/include"
)

# Copy FMOD .so files to jniLibs for APK packaging
set(JNILIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}")
file(MAKE_DIRECTORY ${JNILIBS_DIR})
file(COPY ${FMOD_CORE_LOCATION} DESTINATION ${JNILIBS_DIR})
file(COPY ${FMOD_STUDIO_LOCATION} DESTINATION ${JNILIBS_DIR})

# ----- Include the main project (builds MagicEngine and all dependencies) -----
# This uses the unified submodule-based build system
add_subdirectory("${PROJECT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/magicengine-build")

# ----- Android Native Library -----
# NOTE: native_app_glue is created in the root CMakeLists.txt (ANDROID block)
add_library(${CMAKE_PROJECT_NAME} SHARED
    android_main.cpp
)

# Remove debug 'd' suffix so Java can find the library with a consistent name
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "")

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ----- Vulkan Validation Layers (Debug only) -----
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VALIDATION_LAYER_DIR "${ANDROID_NDK}/sources/third_party/vulkan/src/build-android/jniLibs/${ANDROID_ABI}")
    set(VALIDATION_LAYER_LIB "${VALIDATION_LAYER_DIR}/libVkLayer_khronos_validation.so")

    if(EXISTS "${VALIDATION_LAYER_LIB}")
        message(STATUS "Vulkan validation layer found: ${VALIDATION_LAYER_LIB}")
        add_library(VkLayer_khronos_validation SHARED IMPORTED)
        set_target_properties(VkLayer_khronos_validation PROPERTIES
            IMPORTED_LOCATION "${VALIDATION_LAYER_LIB}"
        )
    else()
        message(WARNING "Vulkan validation layer not found at: ${VALIDATION_LAYER_LIB}")
    endif()
endif()

# ----- Find Android NDK libraries -----
find_library(mediandk-lib mediandk)

# ----- Link everything -----
# Use whole-archive for native_app_glue (ANativeActivity_onCreate entry point)
# and MagicEngine (static initializers for component registration)
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    -Wl,--whole-archive
    native_app_glue    # For ANativeActivity_onCreate - MUST be in whole-archive
    MagicEngine        # For static initializers
    -Wl,--no-whole-archive
    android
    log
    ${mediandk-lib}    # For video playback (MediaCodec NDK)
    fmod_core
    fmod_studio
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    VK_USE_PLATFORM_ANDROID_KHR=1
)

message(STATUS "=== Android native library configured ===")
