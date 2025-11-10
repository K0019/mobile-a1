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

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Editor/Containers/GUICollection.h"
#include "FilepathConstants.h"
#include "graphics/features/grid_feature.h"
#include "Editor/EditorCameraBridge.h"
#include "Graphics/GraphicsECSMesh.h"
#include "Graphics/LightComponent.h"
#include "Engine/Resources/ResourceManager.h"

#include "fa.h"

GraphicsMain::GraphicsMain()
    : sceneFeatureHandle{}
    , gridHandle{}
{
}

GraphicsMain::~GraphicsMain()
{
    if (context.renderer)
    {
        context.renderer->DestroyFeature(gridHandle);
        context.renderer->DestroyFeature(sceneFeatureHandle);
    }
}

void GraphicsMain::Init(Context inContext)
{
	context = inContext;
	sceneFeatureHandle = context.renderer->CreateFeature<SceneRenderFeature>();
	gridHandle = context.renderer->CreateFeature<GridFeature>();

#ifdef IMGUI_ENABLED
	InitImGui(Filepaths::assets + "/Fonts" + "/Lato-Regular.ttf");
	//InitImGui(Filepaths::fontsSave + "/Lato-Regular.ttf");
#endif
}

void GraphicsMain::BeginFrame()
{
}

#ifdef IMGUI_ENABLED
void GraphicsMain::BeginImGuiFrame()
{
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
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		ImGui::End();
	imguiContext->endFrame();
}
#endif

void GraphicsMain::EndFrame(FrameData* outFrameData)
{
	UploadToPipeline(outFrameData);
}

void GraphicsMain::UploadToPipeline(FrameData* outFrameData)
{
    auto params = static_cast<SceneRenderParams*>(context.renderer->GetFeatureParameterBlockPtr(sceneFeatureHandle));
    if (!params)
        return;

    // Clear previous frame data
    params->clear();

    // Estimate size for performance
    size_t estimatedObjects = 0;
    for (auto iter = ecs::GetCompsBegin<RenderComponent>(); iter != ecs::GetCompsEnd<RenderComponent>(); ++iter)
        ++estimatedObjects;
    params->reserve(estimatedObjects);

    // Iterate all RenderComponents in the ECS and populate render params directly
    size_t objectIndex = 0;
    for (auto compIter = ecs::GetCompsBegin<RenderComponent>(); compIter != ecs::GetCompsEnd<RenderComponent>(); ++compIter)
    {
        RenderComponent& comp = *compIter;

        auto mesh = MagicResourceManager::Meshes().GetResource(comp.GetMeshHash());
        if (!mesh)
            continue;

        const auto& materialList = comp.GetMaterialsList();
        size_t materialCount = materialList.size();

        // Process each submesh
        for (size_t i = 0; i < mesh->handles.size(); ++i)
        {
            // Determine material to use
            size_t materialHashToUse = 0;
            if (i < materialCount && materialList[i] != 0)
                materialHashToUse = materialList[i];
            else
                materialHashToUse = mesh->defaultMaterialHashes[i];

            auto material = MagicResourceManager::Materials().GetResource(materialHashToUse);
            if (!material)
                continue;

            const MeshHandle& meshHandle = mesh->handles[i];
            const MaterialHandle& materialHandle = material->handle;

            // Validate handles
            if (!meshHandle.isValid() || !materialHandle.isValid())
                continue;

            const auto* meshData = context.resourceMngr->getMesh(meshHandle);
            if (!meshData)
                continue;

            // Get entity transform
            const Transform& entityTransform = ecs::GetEntityTransform(&comp);
            Mat4 worldTransform = entityTransform.GetWorldMat() * mesh->transforms[i];
            float maxScale = glm::compMax(glm::abs(static_cast<glm::vec3>(entityTransform.GetWorldScale())));

            // Store object data
            params->objectTransforms.push_back(worldTransform);
            params->materialIndices.push_back(context.resourceMngr->getMaterialIndex(materialHandle));

            // Transform mesh bounds to world space
            auto center = vec3(meshData->bounds.x, meshData->bounds.y, meshData->bounds.z);
            float radius = meshData->bounds.w * maxScale;
            auto worldCenter = vec3((glm::mat4)worldTransform * vec4(center, 1.0f));
            params->objectBounds.emplace_back(
                worldCenter - vec3(radius),
                worldCenter + vec3(radius)
            );

            // Build draw command
            DrawIndexedIndirectCommand cmd = {
                .count = meshData->indexCount,
                .instanceCount = 1,
                .firstIndex = static_cast<uint32_t>(meshData->indexByteOffset / sizeof(uint32_t)),
                .baseVertex = static_cast<int32_t>(meshData->vertexByteOffset / sizeof(Vertex)),
                .baseInstance = static_cast<uint32_t>(objectIndex)
            };

            DrawData drawData = {
                .transformId = static_cast<uint32_t>(objectIndex),
                .materialId = context.resourceMngr->getMaterialIndex(materialHandle)
            };

            uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.size());
            params->drawCommands.push_back(cmd);
            params->drawData.push_back(drawData);

            // Sort into opaque/transparent queues
            if (context.resourceMngr->isMaterialTransparent(materialHandle))
                params->transparentIndices.push_back(drawCommandIndex);
            else
                params->opaqueIndices.push_back(drawCommandIndex);

            ++objectIndex;
        }
    }

    // Iterate all LightComponents in the ECS and populate light data directly
    params->lights.clear();
    for (auto lightIter = ecs::GetCompsBegin<LightComponent>(); lightIter != ecs::GetCompsEnd<LightComponent>(); ++lightIter)
    {
        LightComponent& lightComp = *lightIter;
        const SceneLight& sceneLight = lightComp.light;

        // Skip disabled lights
        if (sceneLight.intensity <= 0.0f)
            continue;

        // Skip lights with zero/invalid color
        if (length(sceneLight.color) <= 0.0f)
            continue;

        Lighting::GPULight gpuLight;
        // Convert SceneLight to GPULight (using the same conversion logic as SceneRenderFeature)
        gpuLight.position = sceneLight.position;
        gpuLight.color = sceneLight.color * sceneLight.intensity;
        gpuLight.type = static_cast<uint32_t>(sceneLight.type);
        gpuLight.direction = sceneLight.direction;

        // Calculate range from attenuation
        float constant = sceneLight.attenuation.x;
        float linear = sceneLight.attenuation.y;
        float quadratic = sceneLight.attenuation.z;
        float threshold = 0.01f;
        float maxIntensity = std::max(std::max(sceneLight.color.r, sceneLight.color.g), sceneLight.color.b) * sceneLight.intensity;
        if (quadratic > 0.0f)
        {
            gpuLight.range = (-linear + sqrt(linear * linear - 4.0f * quadratic * (constant - maxIntensity / threshold))) / (2.0f * quadratic);
        }
        else if (linear > 0.0f)
        {
            gpuLight.range = (maxIntensity / threshold - constant) / linear;
        }
        else
        {
            gpuLight.range = 1000.0f; // Default large range
        }

        gpuLight.spotAngle = glm::cos(sceneLight.outerConeAngle);

        params->lights.push_back(gpuLight);
    }

    params->activeLightCount = static_cast<uint32_t>(params->lights.size());

    // Set environment parameters
    params->irradianceTexture = 0;
    params->prefilterTexture = 0;
    params->brdfLUT = 0;
    params->environmentIntensity = 1.0f;

    // Update frame data
    float width = static_cast<float>(Core::Display().GetWidth());
    float height = static_cast<float>(Core::Display().GetHeight());
    outFrameData->cameraPos = frameData.cameraPos;
    outFrameData->viewMatrix = frameData.viewMatrix;
    outFrameData->projMatrix = glm::perspective(45.0f, width / height, 0.1f, 1000.0f);

    EditorCam_Publish(outFrameData->viewMatrix, outFrameData->projMatrix, false);
}

void GraphicsMain::SetViewCamera(const Camera& camera)
{
    frameData.cameraPos = camera.getPosition();
    frameData.viewMatrix = camera.getViewMatrix();
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

#ifdef IMGUI_ENABLED
void GraphicsMain::InitImGui(const std::string& fontfile)
{
	imguiContext = std::make_unique<editor::ImGuiContext>(context);
	SetImGuiStyle();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = "imgui.ini";
	//io.Fonts->AddFontDefault();
	io.Fonts->Clear(); // Clear existing fonts
	constexpr float baseFontSize = 13.0f; // 13.0f is the size of the default font. Change to the font size you use.
	constexpr float iconFontSize = baseFontSize * 2.5f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
	const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true; // Merge icon font to the previous font if you want to have both icons and text
	icons_config.PixelSnapH = true;
	icons_config.GlyphMinAdvanceX = iconFontSize;
	io.Fonts->AddFontFromFileTTF(fontfile.c_str(), baseFontSize);
	io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, iconFontSize, &icons_config, icons_ranges);

	imguiContext->rebuildFontAtlas();
	//If you want change between icons size you will need to create a new font
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 12.0f, &icons_config, icons_ranges);
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 20.0f, &icons_config, icons_ranges);
}

void GraphicsMain::SetImGuiStyle()
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
	style.TabCloseButtonMinWidthUnselected = 0.0f;
	style.TabCloseButtonMinWidthSelected = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
}

editor::ImGuiContext& GraphicsMain::GetImGuiContext()
{
	return *imguiContext.get();
}
#endif // IMGUI_ENABLED

Resource::ResourceManager& GraphicsMain::GetAssetSystem()
{
	assert(context.resourceMngr);
	return *context.resourceMngr;
}
