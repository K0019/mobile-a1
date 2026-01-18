# ImGuizmo CMake wrapper
# Depends on imgui

set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/ImGuizmo)

add_library(imguizmo_lib STATIC
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
    ${IMGUIZMO_DIR}/ImGuizmo.h
    ${IMGUIZMO_DIR}/ImCurveEdit.cpp
    ${IMGUIZMO_DIR}/ImCurveEdit.h
    ${IMGUIZMO_DIR}/ImGradient.cpp
    ${IMGUIZMO_DIR}/ImGradient.h
    ${IMGUIZMO_DIR}/ImSequencer.cpp
    ${IMGUIZMO_DIR}/ImSequencer.h
    ${IMGUIZMO_DIR}/GraphEditor.cpp
    ${IMGUIZMO_DIR}/GraphEditor.h
)

target_include_directories(imguizmo_lib PUBLIC ${IMGUIZMO_DIR})

# Link to imgui
if(TARGET imgui_lib)
    target_link_libraries(imguizmo_lib PUBLIC imgui_lib)
elseif(TARGET imgui::imgui)
    target_link_libraries(imguizmo_lib PUBLIC imgui::imgui)
endif()

# Create alias for consistent naming
add_library(imguizmo::imguizmo ALIAS imguizmo_lib)
