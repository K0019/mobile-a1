# ----- C# Target -----

add_library(EngineScripting SHARED)

# no such thing as debug for this project, remove the postfix
set_target_properties(EngineScripting PROPERTIES
    DEBUG_POSTFIX ""
)

# Auto copy scripting to the bin folder
set_target_properties(EngineScripting PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)

file(GLOB_RECURSE CSHARP_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "*.cs")

target_sources(EngineScripting PRIVATE
    ${CSHARP_IMPLEMENTATION_FILES}
)

engine_set_source_groups(EngineScripting)

set_property(TARGET EngineScripting PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")
set_property(TARGET EngineScripting PROPERTY VS_DOTNET_REFERENCES
    "Microsoft.CSharp"
    "System"
    "System.Core"
    "System.Xml.Linq"
    "System.Data.DataSetExtensions"
    "System.Data"
    "System.Net.Http"
    "System.Xml"
)
set_property(TARGET EngineScripting PROPERTY VS_PACKAGE_REFERENCES "GlmSharp_0.9.8")

install(TARGETS EngineScripting 
    EXPORT MagicEngineTargets
    RUNTIME DESTINATION bin
)