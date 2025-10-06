# ----- Engine Target -----
add_executable(Engine WIN32) #

# Search for files needed
file(GLOB_RECURSE ENGINE_IMPLEMENTATION_FILES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE ENGINE_HEADER_FILES CONFIGURE_DEPENDS "*.h" "*.hpp" "*.ipp")
file(GLOB_RECURSE SHADER_FILES "${CMAKE_SOURCE_DIR}/shaders/*.vert" "${CMAKE_SOURCE_DIR}/shaders/*.frag")
file(GLOB_RECURSE ENGINE_SOURCE_FILES_DIRECTORIES LIST_DIRECTORIES true "*")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_STATIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.lib")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES}/debug/*.dll")
file(GLOB_RECURSE EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES}/release/*.dll")
set(EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES "${EXTERNAL_LIBRARIES}/directory_copy")
# To reformat into a style that copy_if_different can accept
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_DEBUG "${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}")
string(REPLACE ";" "\" \"" EXTERNAL_LIBRARIES_DYNAMIC_RELEASE "${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}")
# To reformat for xcopy
string(REPLACE "/" "\\" COMPILED_SHADER_FILES "${CMAKE_SOURCE_DIR}/Shaders/*.spv")
string(REPLACE "/" "\\" OUTPUT_COMPILED_SHADER_FILES "$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Assets/Shaders>$<$<CONFIG:Release>:$<TARGET_FILE_DIR:Engine>/Assets/Shaders>")

target_sources(Engine PRIVATE
    ${ENGINE_IMPLEMENTATION_FILES}
    ${ENGINE_HEADER_FILES}
    ${SHADER_FILES}
)

target_precompile_headers(Engine PRIVATE pch.h)

target_include_directories(Engine PUBLIC
    ${CMAKE_SOURCE_DIR}/engine
    ${EXTERNAL_INCLUDES}
)
foreach(item ${ENGINE_SOURCE_FILES_DIRECTORIES})
	if(IS_DIRECTORY ${item})
		target_include_directories(Engine PUBLIC ${item})
	endif()
endforeach()

target_link_libraries(Engine PUBLIC
    # --- Core Dependencies (from vcpkg) ---
    # Correct namespaced targets are used as per vcpkg output.
    assimp::assimp
    freeimage::FreeImage 
	freeimage::FreeImagePlus # FreeImage::FreeImagePlus is also available if needed
    Freetype::Freetype
    glfw
    glm::glm-header-only
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
    KTX::ktx
    meshoptimizer::meshoptimizer
	mikktspace::mikktspace
    quill::quill
    Tracy::TracyClient
    volk::volk_headers  # This provides the Vulkan headers
    Vulkan::Vulkan
	Jolt::Jolt

    AssetCompiler

    # Shader compilation
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
    glslang::SPVRemapper
    spirv-cross-core
    spirv-cross-glsl
    unofficial::spirv-reflect
	offsetAllocator
    # External libraries
    $<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_STATIC_DEBUG}>
    $<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_STATIC_RELEASE}>
)

engine_set_source_groups(Engine)

target_compile_options(Engine PRIVATE /W4 /MP /bigobj)
target_compile_definitions(Engine PUBLIC
    TRACY_ENABLE
	GLFW
    -DQUILL_DISABLE_NON_PREFIXED_MACROS
    $<$<CONFIG:Debug>:IMGUI_ENABLED>
)

add_custom_command(TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "\"$<$<CONFIG:Debug>:${EXTERNAL_LIBRARIES_DYNAMIC_DEBUG}>$<$<CONFIG:Release>:${EXTERNAL_LIBRARIES_DYNAMIC_RELEASE}>\""
        "\"$<TARGET_FILE_DIR:Engine>\""
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "\"${EXTERNAL_LIBRARIES_WHOLE_DIRECTORIES}\""
        "\"$<TARGET_FILE_DIR:Engine>\""
    COMMAND cmd.exe /c if "$(Configuration)" == "Release" "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:Engine>/Assets"
    COMMAND cmd.exe /c "${CMAKE_COMMAND}" -E copy_directory_if_different "${CMAKE_SOURCE_DIR}/Shaders" "$<TARGET_FILE_DIR:Engine>/Shaders"
    
)

if(MSVC)
    target_compile_definitions(Engine PUBLIC
        WIN32_LEAN_AND_MEAN
    )
    target_link_options(Engine PUBLIC
        $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
    )
    set_target_properties(Engine PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Engine>"
    )
	set(PORTABLE_LAYERS_PATH "${CMAKE_SOURCE_DIR}/extern/vulkan-debug-layer")
	if(EXISTS "${PORTABLE_LAYERS_PATH}/VkLayer_khronos_validation.json")
		file(TO_NATIVE_PATH "${PORTABLE_LAYERS_PATH}" NATIVE_PORTABLE_LAYERS_PATH)
		message(STATUS "Configuring portable Vulkan layers for MSVC debugging at: ${NATIVE_PORTABLE_LAYERS_PATH}")

		set_property(TARGET Engine PROPERTY
			VS_DEBUGGER_ENVIRONMENT
				"VK_LAYER_PATH=${NATIVE_PORTABLE_LAYERS_PATH}
				 VK_LOADER_DEBUG=all"
		)
	else()
		message(WARNING "Portable Vulkan layer manifest not found at ${PORTABLE_LAYERS_PATH}")
	endif()
endif()
