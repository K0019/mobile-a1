#imgui module

engine_add_module(ryEngine-ImGui
    FOLDER "ryEngine-modules/imgui"
    PUBLIC_DEPENDENCIES
        ryEngine-Core
        imgui::imgui
    DEPENDENCIES
        Freetype::Freetype
	DEFINITIONS
		IMGUI_ENABLE_FREETYPE
)

target_include_directories(ryEngine-ImGui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS ryEngine-ImGui
    EXPORT ryEngineTargets
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
