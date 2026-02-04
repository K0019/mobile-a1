/******************************************************************************/
/*!
\file   GraphicsAPI.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Abstracts the interface to the graphics pipeline.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Editor/Containers/GUICollection.h"
#include "FilepathConstants.h"
#include "renderer/features/grid_feature.h"
#include "renderer/render_feature.h"
#include "Editor/EditorCameraBridge.h"
#include "Graphics/RenderComponent.h"
#include "Graphics/AnimationComponent.h"
#include "Graphics/LightComponent.h"
#include "Assets/AssetManager.h"

#include <algorithm>
#include <iostream>

#include "fa.h"
#include "renderer/im3d_helper.h"
#include "renderer/features/im3d_feature.h"
#include "renderer/features/ui2d_render_feature.h"
#include "VFS/VFS.h"

#include "ECS/ECSSysLayers.h"
#include "Utilities/Messaging.h"
#include "Game/GameSystems.h"

#include "stb_image.h"
#include "math/utils_cubemap.h"
#include "resource/resource_types.h"

GraphicsMain::GraphicsMain()
	: sceneFeatureHandle{}
	, ui2dFeatureHandle{}
	, gridHandle{}
	, im3dHandle{}
{
}

GraphicsMain::~GraphicsMain()
{
	Messaging::Unsubscribe("EngineTogglePlayMode", OnTogglePlayMode);

	// Features are now owned by the Application - it will destroy them
	// We just need to clear our references
	gridHandle = 0;
	sceneFeatureHandle = 0;
	ui2dFeatureHandle = 0;
	im3dHandle = 0;
}

void GraphicsMain::Init(Context inContext)
{
	context = inContext;
	// Features are now created by the Application layer
	// Application will call SetSceneFeatureHandle(), SetGridFeatureHandle(), etc.
	// ImGui is now Application-controlled - Editor calls InitializeImGui(), Game does not

	const std::string fontsFile{ Filepaths::fontsSave + "/lato-regular.ttf" };

	InitFont(fontsFile);
	InitDefaultSkybox();

	// Subscribe to play mode toggle events
	Messaging::Subscribe("EngineTogglePlayMode", OnTogglePlayMode);
}

void GraphicsMain::InitializeImGui(const std::string& fontfile)
{
	InitImGui(fontfile);
}

void GraphicsMain::InitializeUI2DOverlay()
{
	// Called by Application after setting UI2D feature handle
	if (ui2dFeatureHandle != 0 && context.renderer && context.resourceMngr)
	{
		overlayGui = std::make_unique<ui::ImmediateGui>(*context.renderer, ui2dFeatureHandle, *context.resourceMngr);
	}
}

void GraphicsMain::BeginFrame()
{
	Im3dHelper::BeginFrame({});
}

void GraphicsMain::BeginImGuiFrame()
{
	// Skip if ImGui wasn't initialized (Game build doesn't call InitializeImGui)
	if (!imguiContext) return;

	imguiContext->beginFrame();

	ImGuiIO& io = ImGui::GetIO();
	if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
		return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##DockSpace ", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

void GraphicsMain::EndImGuiFrame()
{
	// Skip if ImGui wasn't initialized (Game build doesn't call InitializeImGui)
	if (!imguiContext) return;

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		ImGui::End();
	imguiContext->endFrame();
}

void GraphicsMain::EndFrame(FrameData* outFrameData)
{
	Im3dHelper::EndFrame(im3dHandle, *context.renderer);
	UploadToPipeline(outFrameData);
}

void GraphicsMain::EndFrame(RenderFrameData* outRenderFrameData)
{
	Im3dHelper::EndFrame(im3dHandle, *context.renderer);

	// Ensure camera matrices are valid before populating views
	// This handles the case where SetViewCamera hasn't been called yet (e.g., first frame)
	if (frameData.viewMatrix == glm::mat4(1.0f) || frameData.viewMatrix == glm::mat4(0.0f))
	{
		// Default camera: looking down -Z axis from slightly behind origin
		frameData.cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
		frameData.viewMatrix = glm::lookAt(
			frameData.cameraPos,           // eye position
			glm::vec3(0.0f, 0.0f, 0.0f),   // look at origin
			glm::vec3(0.0f, 1.0f, 0.0f)    // up vector
		);
	}

	if (frameData.projMatrix == glm::mat4(1.0f) || frameData.projMatrix == glm::mat4(0.0f))
	{
		float width = static_cast<float>(Core::Display().GetWidth());
		float height = static_cast<float>(Core::Display().GetHeight());
		float aspect = (height > 0.0f) ? (width / height) : (16.0f / 9.0f);
		frameData.projMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
	}

	// Populate ALL views with camera matrices from GraphicsMain::frameData
	// This ensures every view that runs scene rendering has valid camera data
	if (outRenderFrameData)
	{
		for (auto& view : outRenderFrameData->views)
		{
			view.cameraPos = frameData.cameraPos;
			view.viewMatrix = frameData.viewMatrix;
			view.projMatrix = frameData.projMatrix;
		}
	}

	// Run UploadToPipeline without a single outFrameData (we already populated views above)
	UploadToPipeline(nullptr);
}

void GraphicsMain::InitFont(const std::string& fontfile)
{
	std::vector<uint8_t> fontBytes{};
	if (!VFS::ReadFile(fontfile, fontBytes))
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to initialize font: " << fontfile;
		return;
	}

	Resource::ProcessedFont processed;
	processed.name = "UI2D_Font";
	processed.fontFileData = std::move(fontBytes);
	processed.buildSettings.pixelHeight = 20.0f;
	processed.buildSettings.firstCodepoint = 32;
	processed.buildSettings.lastCodepoint = 255;
	processed.buildSettings.fallbackCodepoint = '?';
	processed.sourceFile = fontfile;
	processed.buildSettings.extraCodepoints.push_back(0x2026u); // Ellipsis
	ui2dFontHandle = context.resourceMngr->createFont(processed);
}

void GraphicsMain::InitDefaultSkybox()
{
	const std::string skyboxPath = "images/skybox.png";

	// Read the image file via VFS
	std::vector<uint8_t> fileData;
	if (!VFS::ReadFile(skyboxPath, fileData))
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Failed to load default skybox: " << skyboxPath;
		return;
	}

	// Decode using stb_image
	int width, height, channels;
	unsigned char* pixels = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &width, &height, &channels, 4);
	if (!pixels)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Failed to decode skybox image: " << skyboxPath;
		return;
	}

	// Convert to Bitmap for cubemap conversion
	Bitmap equirect(width, height, 4, eBitmapFormat_UnsignedByte, pixels);
	stbi_image_free(pixels);

	// Convert equirectangular to cubemap faces
	// Output: cubemap.w_ = face width, cubemap.h_ = face height * 6 (stacked vertically)
	Bitmap cubemap = convertEquirectangularMapToCubeMapFaces(equirect);
	if (cubemap.data_.empty())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Failed to convert skybox to cubemap";
		return;
	}

	// The cubemap Bitmap has:
	// - w_ = face width
	// - h_ = face height (each face, NOT total height)
	// - d_ = 6 (number of faces)
	// Data is arranged as 6 contiguous faces
	uint32_t faceWidth = static_cast<uint32_t>(cubemap.w_);
	uint32_t faceHeight = static_cast<uint32_t>(cubemap.h_);

	CONSOLE_LOG(LEVEL_INFO) << "Skybox cubemap: face size " << faceWidth << "x" << faceHeight
	                        << ", total data size " << cubemap.data_.size() << " bytes";

	// Create cubemap texture directly via hina-vk (bypasses ResourceManager which doesn't support cubemaps)
	hina_texture_desc desc = hina_texture_desc_default();
	desc.type = HINA_TEX_TYPE_CUBE;
	desc.format = HINA_FORMAT_R8G8B8A8_SRGB;  // sRGB for color data
	desc.width = faceWidth;
	desc.height = faceHeight;
	desc.depth = 1;
	desc.layers = 6;  // Cubemap has 6 faces
	desc.mip_levels = 1;
	desc.samples = HINA_SAMPLE_COUNT_1_BIT;
	desc.usage = HINA_TEXTURE_SAMPLED_BIT;
	desc.initial_data = cubemap.data_.data();
	desc.initial_stride = 0;  // Tight packing
	desc.label = "DefaultSkyboxCubemap";

	gfx::Texture skyboxTex = hina_make_texture(&desc);
	if (!hina_texture_is_valid(skyboxTex))
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Failed to create skybox cubemap texture via hina-vk";
		return;
	}

	gfx::TextureView skyboxView = hina_texture_get_default_view(skyboxTex);
	if (!hina_texture_view_is_valid(skyboxView))
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Failed to get skybox texture view";
		hina_destroy_texture(skyboxTex);
		return;
	}

	// Set the skybox on the renderer for scene feature to use
	if (context.renderer)
	{
		context.renderer->setSkyboxTexture(skyboxTex, skyboxView);
		CONSOLE_LOG(LEVEL_INFO) << "Default skybox cubemap (" << faceWidth << "x" << faceHeight << ") loaded and set on renderer";
	}
}

void GraphicsMain::UploadToPipeline(FrameData* outFrameData)
{
	// Populate outFrameData with camera matrices from GraphicsMain::frameData
	// (frameData is populated by SetViewCamera() which is called by camera systems)
	if (outFrameData)
	{
		outFrameData->cameraPos = frameData.cameraPos;
		outFrameData->viewMatrix = frameData.viewMatrix;
		outFrameData->projMatrix = frameData.projMatrix;  // Now set by SetViewCamera
	}

	// Set up frustum planes for CPU culling
	glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
	context.renderer->setFrustum(viewProj);

	// Submit ECS RenderComponents to GfxRenderer via SceneRenderFeature
	SceneRenderFeature::UpdateScene(sceneFeatureHandle, *context.resourceMngr, *context.renderer);

	// 2D UI pass - queue UI commands from ECS systems
	if (overlayGui && overlayGui->begin(ui2dFontHandle)) {
		float width = static_cast<float>(Core::Display().GetWidth());
		float height = static_cast<float>(Core::Display().GetHeight());
		overlayGui->setViewport(width, height);
		ecs::RunSystemsInLayers(ECS_LAYER::CUTOFF_RENDER, ECS_LAYER::CUTOFF_RENDER_UI);
		overlayGui->end();
	}

	// Scene rendering handled exclusively by SceneRenderFeature (deferred rendering path)
	// See: MagicEngine/renderer/features/scene_feature.cpp
}

void GraphicsMain::SetViewCamera(const Camera& camera)
{
	frameData.cameraPos = camera.getPosition();
	frameData.viewMatrix = camera.getViewMatrix();

	// Use camera's projection if set, otherwise calculate default perspective
	glm::mat4 camProj = camera.getProjMatrix();
	if (camProj == glm::mat4(0.0f) || camProj == glm::mat4(1.0f)) {
		// Camera projection not set - calculate default
		float width = static_cast<float>(Core::Display().GetWidth());
		float height = static_cast<float>(Core::Display().GetHeight());
		if (height > 0.0f) {
			frameData.projMatrix = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f);
		} else {
			// Fallback: use default 16:9 aspect ratio if display not ready
			frameData.projMatrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
		}
	} else {
		frameData.projMatrix = camProj;
	}

	// Vulkan Y-flip: Vulkan NDC has Y pointing down (unlike OpenGL which points up).
	// glm::perspective() produces OpenGL-style projection, so we flip Y to correct for Vulkan.
	// This also preserves correct winding order for back-face culling.
	frameData.projMatrix[1][1] *= -1.0f;
}

void GraphicsMain::SetGridEnabled(bool enabled)
{
	// The render feature uses triple buffering - we need to set all buffers
	// to ensure the change propagates immediately without flickering
	auto* gridFeature = context.renderer->GetFeature<GridFeature>(gridHandle);
	if (gridFeature)
	{
		// Write to all parameter buffers
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			gridFeature->param_[i].enabled = enabled;
		}
	}
}

void GraphicsMain::OnTogglePlayMode()
{
	auto* gameStateManager = ST<GameSystemsManager>::Get();
	// When message is broadcast, state hasn't changed yet
	// So if currently in EDITOR, we're about to go to IN_GAME (disable grid)
	// If currently in IN_GAME/PAUSE, we're about to go to EDITOR (enable grid)
	bool currentlyInEditor = (gameStateManager->GetState() == GAMESTATE::EDITOR);
	ST<GraphicsMain>::Get()->SetGridEnabled(!currentlyInEditor);
}

FrameData& GraphicsMain::INTERNAL_GetFrameData()
{
	return frameData;
}

void GraphicsMain::SetPendingShutdown()
{
	ST<GraphicsWindow>::Get()->SetPendingShutdown();
}
bool GraphicsMain::GetIsPendingShutdown() const
{
	return ST<GraphicsWindow>::Get()->GetIsPendingShutdown();
}

void GraphicsMain::InitImGui(const std::string& fontfile)
{
	// Get DPI scale factor for high-DPI displays
	// This is used to render fonts at higher resolution for crisp text,
	// while keeping the UI at the same logical size as 1920x1080
#ifndef __ANDROID__
	const float dpiScale = Core::Display().GetContentScale();
#else
	const float dpiScale = 1.0f;
#endif

	// Create ImGui context - displayScale=1.0 keeps UI at reference size
	ImGuiConfig imguiConfig;
	imguiConfig.displayScale = 1.0f;  // Don't scale the logical UI size
	imguiContext = std::make_unique<editor::ImGuiContext>(context, imguiConfig);
	SetImGuiStyle();  // Use original unscaled style values

	ImGuiIO& io = ImGui::GetIO();
#ifdef __ANDROID__
	io.IniFilename = nullptr;  // Disable imgui.ini on Android (no writable path)
#else
	io.IniFilename = "imgui.ini";
#endif
	//io.Fonts->AddFontDefault();
	io.Fonts->Clear(); // Clear existing fonts

	// Load fonts at higher resolution for crisp rendering on high-DPI displays,
	// then scale down via FontGlobalScale to maintain consistent logical size
	constexpr float baseFontSizeUnscaled = 20.0f; // 13.0f is the size of the default font
	const float baseFontSize = baseFontSizeUnscaled * dpiScale;  // Render at higher res
	const float iconFontSize = baseFontSize * 2.5f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
	const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true; // Merge icon font to the previous font if you want to have both icons and text
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = iconFontSize;

#ifdef __ANDROID__
	// On Android, we must read fonts from assets into memory
	std::vector<uint8_t> fontData;
	if (VFS::ReadFile(fontfile, fontData) && !fontData.empty())
	{
		// ImGui takes ownership of the font data when using AddFontFromMemoryTTF
		// so we need to allocate with IM_ALLOC
		void* fontDataCopy = IM_ALLOC(fontData.size());
		memcpy(fontDataCopy, fontData.data(), fontData.size());
		io.Fonts->AddFontFromMemoryTTF(fontDataCopy, static_cast<int>(fontData.size()), baseFontSize);
	}
	else
	{
		// Fallback to default font if font file not found
		LOG_WARNING("Could not load font '{}', using default", fontfile);
		io.Fonts->AddFontDefault();
	}
#else
	const std::string physicalFilepath{ VFS::ConvertVirtualToPhysical(fontfile) };
	io.Fonts->AddFontFromFileTTF(physicalFilepath.c_str(), baseFontSize);
#endif

	io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, iconFontSize, &icons_config, icons_ranges);

	// Scale fonts back down so they appear at the original logical size
	// but are rendered with higher-resolution glyphs for crispness
	io.FontGlobalScale = 1.0f / dpiScale;

	imguiContext->rebuildFontAtlas();
	//If you want change between icons size you will need to create a new font
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 12.0f, &icons_config, icons_ranges);
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 20.0f, &icons_config, icons_ranges);
}

void GraphicsMain::SetImGuiStyle(float /*dpiScale*/)
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
	colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
	colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.300000011920929f;
	// Original style values - same logical size as 1920x1080
	style.WindowPadding = ImVec2(10.10000038146973f, 10.10000038146973f);
	style.WindowRounding = 10.30000019073486f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(20.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.ChildRounding = 8.199999809265137f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.69999980926514f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(20.0f, 1.5f);
	style.FrameRounding = 4.800000190734863f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(9.699999809265137f, 5.300000190734863f);
	style.ItemInnerSpacing = ImVec2(5.400000095367432f, 9.300000190734863f);
	style.CellPadding = ImVec2(7.900000095367432f, 2.0f);
	style.IndentSpacing = 10.69999980926514f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 12.10000038146973f;
	style.ScrollbarRounding = 20.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 4.599999904632568f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
}

editor::ImGuiContext& GraphicsMain::GetImGuiContext()
{
	return *imguiContext.get();
}

Resource::ResourceManager& GraphicsMain::GetAssetSystem()
{
	assert(context.resourceMngr);
	return *context.resourceMngr;
}

ui::ImmediateGui& GraphicsMain::GetImmediateGui()
{
	return *overlayGui;
}

bool GraphicsMain::RequestObjPick(int mouseX, int mouseY) {
	auto* sceneFeature = context.renderer->GetFeature<SceneRenderFeature>(sceneFeatureHandle);
	if (sceneFeature) {
		sceneFeature->RequestObjectPick(mouseX, mouseY);
		return true;
	}
	return false;
}

ecs::EntityHandle GraphicsMain::PreviousPick() {
	ecs::EntityHandle pickedEntity = nullptr;
	auto* sceneFeature = context.renderer->GetFeature<SceneRenderFeature>(sceneFeatureHandle);
	if (sceneFeature) {
		auto pickResult = sceneFeature->GetLastPickResult();

		if (pickResult.valid)
		{
			lastPickedObjectIndex = pickResult.sceneObjectIndex;

			// Use SceneRenderFeature's entity handle tracking
			pickedEntity = sceneFeature->GetEntityAtDrawIndex(pickResult.sceneObjectIndex);

			// Clear so we don't process again next frame
			sceneFeature->ClearPickResult();
		}
		return pickedEntity;
	}
	return nullptr;
}
