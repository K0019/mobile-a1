# ImPlot CMake wrapper
# Depends on imgui

set(IMPLOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/implot)

add_library(implot_lib STATIC
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot.h
    ${IMPLOT_DIR}/implot_demo.cpp
    ${IMPLOT_DIR}/implot_internal.h
    ${IMPLOT_DIR}/implot_items.cpp
)

target_include_directories(implot_lib PUBLIC ${IMPLOT_DIR})

# Link to imgui
if(TARGET imgui_lib)
    target_link_libraries(implot_lib PUBLIC imgui_lib)
elseif(TARGET imgui::imgui)
    target_link_libraries(implot_lib PUBLIC imgui::imgui)
endif()

# Create alias for consistent naming
add_library(implot::implot ALIAS implot_lib)
