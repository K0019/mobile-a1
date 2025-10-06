# cmake/EngineUtils.cmake
# Common utility functions for the engine build

function(engine_set_folder target folder_name)
    set_property(TARGET ${target} PROPERTY FOLDER ${folder_name})
endfunction()

function(engine_set_source_groups target_name)
    get_target_property(target_sources ${target_name} SOURCES)
    foreach(source ${target_sources})
        get_filename_component(source_path "${source}" PATH)
        file(RELATIVE_PATH rel_source_path ${PROJECT_SOURCE_DIR} ${source_path})
        if(rel_source_path)
            string(REPLACE "\\" "/" source_group_path "${rel_source_path}")
            source_group("${source_group_path}" FILES "${source}")
        endif()
    endforeach()
endfunction()

function(engine_add_module module_name)
    set(options)
    set(oneValueArgs FOLDER)
    set(multiValueArgs EXTRA_SOURCES DEPENDENCIES PUBLIC_DEPENDENCIES DEFINITIONS)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Glob all sources from current directory
    file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS 
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    )
    
    add_library(${module_name} STATIC)
    
    # Add globbed sources + any extra sources
    target_sources(${module_name} PRIVATE ${MODULE_SOURCES} ${MODULE_EXTRA_SOURCES})
    
    if(MODULE_DEPENDENCIES)
        target_link_libraries(${module_name} PRIVATE ${MODULE_DEPENDENCIES})
    endif()
    
    if(MODULE_PUBLIC_DEPENDENCIES)
        target_link_libraries(${module_name} PUBLIC ${MODULE_PUBLIC_DEPENDENCIES})
    endif()
    
    if(MODULE_DEFINITIONS)
        target_compile_definitions(${module_name} PUBLIC ${MODULE_DEFINITIONS})
    endif()
    
    if(MODULE_FOLDER)
        engine_set_folder(${module_name} ${MODULE_FOLDER})
    endif()
    
    engine_set_source_groups(${module_name})
endfunction()

# Create Android native app glue library if needed
function(engine_setup_android_native_app_glue)
    if(NOT TARGET native_app_glue)
        set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
        
        if(NOT EXISTS ${NATIVE_APP_GLUE_DIR})
            message(FATAL_ERROR "Android native app glue not found at: ${NATIVE_APP_GLUE_DIR}")
        endif()
        
        # Create as OBJECT library instead of STATIC to avoid export issues
        add_library(native_app_glue OBJECT
            ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
        )
        
        target_include_directories(native_app_glue PUBLIC
            ${NATIVE_APP_GLUE_DIR}
        )
        
        # Find required Android libraries
        find_library(android-lib android)
        find_library(log-lib log)
        
        target_link_libraries(native_app_glue 
            ${android-lib} 
            ${log-lib}
        )
        
        # Set folder for organization
        engine_set_folder(native_app_glue "Android")
        
        # Make the object files available globally
        set(NATIVE_APP_GLUE_OBJECTS $<TARGET_OBJECTS:native_app_glue> CACHE INTERNAL "Native app glue object files")
        
        message(STATUS "Android native app glue library created")
    endif()
endfunction()

# Platform configuration helper
# Platform configuration helper
function(engine_configure_platform target)
    if(ANDROID)
        # Ensure native app glue is available
        engine_setup_android_native_app_glue()
        
        # Find Android libraries
        find_library(android-lib android)
        find_library(log-lib log)
        find_library(egl-lib EGL)
        
        # Add native app glue objects directly to the target instead of linking
        target_sources(${target} PRIVATE ${NATIVE_APP_GLUE_OBJECTS})
        
        # Link Android-specific libraries (but not native_app_glue)
        target_link_libraries(${target} PRIVATE 
            ${android-lib} 
            ${log-lib}
            ${egl-lib}
        )
        
        # Android compile definitions
        target_compile_definitions(${target} PUBLIC 
            __ANDROID__
            VK_USE_PLATFORM_ANDROID_KHR=1
            ANDROID_PLATFORM=${ANDROID_PLATFORM}
        )
        
        # Include Android NDK headers
        target_include_directories(${target} PRIVATE
            ${ANDROID_NDK}/sources/android/native_app_glue
        )
        
    else()
        # Desktop configuration remains the same
        target_link_libraries(${target} PRIVATE glfw)
        target_compile_definitions(${target} PUBLIC GLFW_ENABLED)
        
        if(WIN32)
            target_compile_definitions(${target} PRIVATE 
                VK_USE_PLATFORM_WIN32_KHR=1 
                NOMINMAX 
                WIN32_LEAN_AND_MEAN
            )
        elseif(UNIX AND NOT APPLE)
            target_compile_definitions(${target} PRIVATE 
                VK_USE_PLATFORM_XLIB_KHR=1
            )
        elseif(APPLE)
            target_compile_definitions(${target} PRIVATE 
                VK_USE_PLATFORM_MACOS_MVK=1
            )
        endif()
    endif()
endfunction()