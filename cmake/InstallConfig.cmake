function(install_config projectName)

    # for CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_INCLUDEDIR and others
    include(GNUInstallDirs)

    # note that ${public_headers} should be in quotes
    set_target_properties(${projectName} PROPERTIES PUBLIC_HEADER "${public_headers}")

    # install the target and create export-set
    install(TARGETS ${projectName}
        EXPORT "${projectName}Targets"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${projectName}" # include/SomeProject
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} # include
    )

    # generate and install export file
    install(EXPORT "${projectName}Targets"
        FILE "${projectName}Targets.cmake"
        NAMESPACE ${namespace}::
        DESTINATION "share/${projectName}"
    )

    include(CMakePackageConfigHelpers)

    # generate the version file for the config file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${projectName}ConfigVersion.cmake"
        COMPATIBILITY AnyNewerVersion
    )
    # create config file
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${projectName}Config.cmake"
        INSTALL_DESTINATION "share/${projectName}"
    )
    # install config files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${projectName}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${projectName}ConfigVersion.cmake"
        DESTINATION "share/${projectName}"
    )

endfunction()
