cmake_minimum_required(VERSION 3.18)
#set(CMAKE_FIND_DEBUG_MODE ON)
project("ryEngineAndroid")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_link_options(-Wl,-z,max-page-size=16384)

message(STATUS "=== Android ryEngine Integration ===")
message(STATUS "Android ABI: ${ANDROID_ABI}")
message(STATUS "Initial CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

# Map Android ABI to the directory structure we created
if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(ABI_SUBDIR "arm64-v8a")
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(ABI_SUBDIR "armeabi-v7a")
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(ABI_SUBDIR "x86_64")
elseif(ANDROID_ABI STREQUAL "x86")
    set(ABI_SUBDIR "x86")
else()
    message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
endif()

# Use the first path from CMAKE_PREFIX_PATH but fix the android subfolder
list(GET CMAKE_PREFIX_PATH 0 LIBRARY_INSTALL_PATH)

set(ABI_SPECIFIC_PATH "${LIBRARY_INSTALL_PATH}/${ABI_SUBDIR}")

message(STATUS "ABI subdirectory: ${ABI_SUBDIR}")
message(STATUS "Library install path: ${LIBRARY_INSTALL_PATH}")
message(STATUS "Looking for libraries in: ${ABI_SPECIFIC_PATH}")

# Add library search path for libktx.a and other installed libraries
link_directories("${ABI_SPECIFIC_PATH}/lib")

# Check if the ABI-specific directory exists
if(NOT EXISTS "${ABI_SPECIFIC_PATH}")
    message(FATAL_ERROR "ABI-specific directory not found: ${ABI_SPECIFIC_PATH}")
endif()

# Replace the first CMAKE_PREFIX_PATH entry with our corrected one
list(REMOVE_AT CMAKE_PREFIX_PATH 0)
list(PREPEND CMAKE_PREFIX_PATH "${ABI_SPECIFIC_PATH}")

message(STATUS "Final CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "Looking for configs in: ${ABI_SPECIFIC_PATH}/share/")

# Check if the actual config file exists
set(CONFIG_FILE "${ABI_SPECIFIC_PATH}/share/MagicEngine/MagicEngineConfig.cmake")
if(EXISTS "${CONFIG_FILE}")
    message(STATUS "✓ Found MagicEngine config file: ${CONFIG_FILE}")
else()
    message(STATUS "✗ MagicEngine config file NOT found: ${CONFIG_FILE}")
    # List what's actually in the share directory
    file(GLOB SHARE_CONTENTS "${ABI_SPECIFIC_PATH}/share/*")
    message(STATUS "Contents of share directory: ${SHARE_CONTENTS}")
endif()

# Set CMAKE_PREFIX_PATH so dependencies can be found
list(REMOVE_AT CMAKE_PREFIX_PATH 0)
list(PREPEND CMAKE_PREFIX_PATH "${ABI_SPECIFIC_PATH}")

# ALSO set the explicit directory for ryEngine
set(magicEngine_DIR "${ABI_SPECIFIC_PATH}/share/MagicEngine")
set(unofficial-spirv-reflect_DIR "${ABI_SPECIFIC_PATH}/share/unofficial-spirv-reflect")

file(GLOB PACKAGE_DIRS "${ABI_SPECIFIC_PATH}/share/*")
foreach(PACKAGE_DIR ${PACKAGE_DIRS})
    get_filename_component(PACKAGE_NAME ${PACKAGE_DIR} NAME)
    # Handle packages with dashes in names
    string(REPLACE "-" "_" PACKAGE_VAR ${PACKAGE_NAME})
    set(${PACKAGE_VAR}_DIR ${PACKAGE_DIR})
endforeach()

# Grab FMOD
# From android/app/src/main/cpp/ up to project root
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
set(FMOD_ROOT "${PROJECT_ROOT}/extern/lib/fmodAndroid")
set(FMOD_CORE_LOCATION "${FMOD_ROOT}/${ANDROID_ABI}/libfmod.so")
set(FMOD_STUDIO_LOCATION "${FMOD_ROOT}/${ANDROID_ABI}/libfmodstudio.so")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "PROJECT_ROOT: ${PROJECT_ROOT}")
message(STATUS "FMOD_ROOT: ${FMOD_ROOT}")

if(NOT EXISTS "${FMOD_CORE_LOCATION}")
    message(FATAL_ERROR "FMOD core not found at ${FMOD_CORE_LOCATION}")
endif()

if(NOT EXISTS "${FMOD_STUDIO_LOCATION}")
    message(FATAL_ERROR "FMOD studio not found at ${FMOD_STUDIO_LOCATION}")
endif()

message(STATUS "FMOD core found ✓")
message(STATUS "FMOD studio found ✓")

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

# Copy .so for FMOD to the JNILibs for packaging into the apk.
set(JNILIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}")
file(MAKE_DIRECTORY ${JNILIBS_DIR})
file(COPY ${FMOD_CORE_LOCATION} DESTINATION ${JNILIBS_DIR})
file(COPY ${FMOD_STUDIO_LOCATION} DESTINATION ${JNILIBS_DIR})

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "magicEngine_DIR: ${magicEngine_DIR}")

find_package(MagicEngine REQUIRED)
message(STATUS "=========================")

# Add native_app_glue as an OBJECT library (recommended modern approach)
set(NATIVE_APP_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")

if(NOT EXISTS "${NATIVE_APP_GLUE_DIR}")
    message(FATAL_ERROR "Could not find native_app_glue directory: ${NATIVE_APP_GLUE_DIR}")
endif()

message(STATUS "Native app glue directory: ${NATIVE_APP_GLUE_DIR}")

add_library(native_app_glue OBJECT
    ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
)

target_include_directories(native_app_glue PUBLIC
    ${NATIVE_APP_GLUE_DIR}
)

# Create the Android native library with sources
add_library(${CMAKE_PROJECT_NAME} SHARED
    android_main.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add engine exported headers
    ${ABI_SPECIFIC_PATH}/include/ryEngine
    # For nice #include "engine.h", and so forth
    ${ABI_SPECIFIC_PATH}/include/ryEngine/core/engine
        ${ABI_SPECIFIC_PATH}/include
        ${ABI_SPECIFIC_PATH}/include/offset_allocator
        ${PROJECT_ROOT}/MagicEngine-core
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    MagicEngine::MagicEngine
    native_app_glue
    android
    log
    fmod_core
    fmod_studio
)

# Android-specific compile definitions
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    VK_USE_PLATFORM_ANDROID_KHR=1
)

message(STATUS "Android native library configured successfully")