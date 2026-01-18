# Dear ImGui CMake wrapper
# Builds imgui as a static library with platform-specific backends

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)

# Core ImGui sources
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/imstb_truetype.h
)

# Vulkan backend (all platforms)
list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp)
list(APPEND IMGUI_HEADERS ${IMGUI_DIR}/backends/imgui_impl_vulkan.h)

# Platform-specific backends
if(ANDROID)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_android.cpp)
    list(APPEND IMGUI_HEADERS ${IMGUI_DIR}/backends/imgui_impl_android.h)
else()
    # Desktop uses GLFW
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp)
    list(APPEND IMGUI_HEADERS ${IMGUI_DIR}/backends/imgui_impl_glfw.h)
endif()

# FreeType support
if(TARGET freetype OR TARGET Freetype::Freetype)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp)
    list(APPEND IMGUI_HEADERS ${IMGUI_DIR}/misc/freetype/imgui_freetype.h)
    set(IMGUI_ENABLE_FREETYPE ON)
endif()

add_library(imgui_lib STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/freetype
)

# Link dependencies
if(ANDROID)
    # Android backend needs android lib
    target_link_libraries(imgui_lib PRIVATE android log)
else()
    # Desktop needs GLFW
    if(TARGET glfw)
        target_link_libraries(imgui_lib PRIVATE glfw)
    endif()
endif()

# Vulkan headers
if(TARGET Vulkan::Headers)
    target_link_libraries(imgui_lib PRIVATE Vulkan::Headers)
elseif(TARGET volk::volk_headers)
    target_link_libraries(imgui_lib PRIVATE volk::volk_headers)
endif()

# FreeType
if(IMGUI_ENABLE_FREETYPE)
    target_compile_definitions(imgui_lib PUBLIC IMGUI_ENABLE_FREETYPE)
    if(TARGET Freetype::Freetype)
        target_link_libraries(imgui_lib PRIVATE Freetype::Freetype)
    elseif(TARGET freetype)
        target_link_libraries(imgui_lib PRIVATE freetype)
    endif()
endif()

# Compile definitions
target_compile_definitions(imgui_lib PUBLIC
    IMGUI_IMPL_VULKAN_NO_PROTOTYPES  # We use volk
)

# Create alias for consistent naming
add_library(imgui::imgui ALIAS imgui_lib)
