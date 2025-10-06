# ----- C# Target -----

add_library(EngineScripting SHARED)

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
