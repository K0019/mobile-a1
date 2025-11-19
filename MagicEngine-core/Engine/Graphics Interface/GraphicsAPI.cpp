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
#include "Graphics/RenderComponent.h"
#include "Graphics/AnimationComponent.h"
#include "Graphics/LightComponent.h"
#include "Engine/Resources/ResourceManager.h"

#include "fa.h"
#include "graphics/im3d_helper.h"
#include "graphics/features/im3d_feature.h"
#include "graphics/features/ui2d_render_feature.h"
#include "graphics/ui/ui_immediate.h"
#include "VFS/VFS.h"

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
		context.renderer->DestroyFeature(ui2dFeatureHandle);
		context.renderer->DestroyFeature(im3dHandle);
	}
}

void GraphicsMain::Init(Context inContext)
{
	context = inContext;
	sceneFeatureHandle = context.renderer->CreateFeature<SceneRenderFeature>();
	gridHandle = context.renderer->CreateFeature<GridFeature>();
	ui2dFeatureHandle = context.renderer->CreateFeature<Ui2DRenderFeature>();
	im3dHandle = context.renderer->CreateFeature<Im3dRenderFeature>();

#ifdef IMGUI_ENABLED
	InitImGui(Filepaths::fontsSave + "/Lato-Regular.ttf");
#endif
}

void GraphicsMain::BeginFrame()
{
	Im3dHelper::BeginFrame({});
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
	Im3dHelper::EndFrame(im3dHandle, *context.renderer);
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
	params->reserve(ecs::GetCompsActiveCount<RenderComponent>());

	// Iterate all RenderComponents in the ECS and populate render params directly
	size_t objectIndex = 0;
	uint32_t animatedVertexCursor = 0;

	for (auto compIter = ecs::GetCompsActiveBegin<RenderComponent>(); compIter != ecs::GetCompsEnd<RenderComponent>(); ++compIter)
	{
		RenderComponent& comp = *compIter;

		auto mesh = comp.GetMesh();
		if (!mesh)
			continue;

		const auto& materialList = comp.GetMaterialsList();
		size_t materialCount = materialList.size();


		// Do we have animations on this object
		auto animComp = ecs::GetEntity(&comp)->GetComp<AnimationComponent>();


		// Process each submesh
		for (size_t i = 0; i < mesh->handles.size(); ++i)
		{
			// Determine material to use
			const ResourceMaterial* material{ (i < materialCount ? materialList[i].GetResource() : nullptr) };
			if (!material)
				material = MagicResourceManager::GetContainer<ResourceMaterial>().GetResource(mesh->defaultMaterialHashes[i]);
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
			const bool hasMorphTargets = meshData->animation.morphDeltaByteOffset != UINT32_MAX;
			if (hasMorphTargets)
			{
				constexpr float MORPH_BOUNDS_EXPANSION = 2.0f;
				radius *= MORPH_BOUNDS_EXPANSION;
			}
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
				.baseVertex = static_cast<int32_t>(meshData->vertexByteOffset / sizeof(CompressedVertex)),
				.baseInstance = static_cast<uint32_t>(objectIndex)
			};

			DrawData drawData = {
				.transformId = static_cast<uint32_t>(objectIndex),
				.materialId = context.resourceMngr->getMaterialIndex(materialHandle),
				.meshDecompIndex = static_cast<uint32_t>(meshData->decompressionByteOffset / sizeof(MeshDecompressionData)),
				.objectId = static_cast<uint32_t>(objectIndex)
			};

			uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.size());
			params->drawCommands.push_back(cmd);
			params->drawData.push_back(drawData);

			// Animation binding
			const auto* meshMetadata = GetAssetSystem().getMeshMetadata(mesh->handles[i]);
			const auto& skeleton = GetAssetSystem().Skeleton(meshMetadata->skeletonId);
			uint32_t jointCount = GetAssetSystem().Skeleton(meshMetadata->skeletonId).jointCount();
			uint32_t morphCount = GetAssetSystem().Morph(meshMetadata->morphSetId).count();
			bool objectAnimated = (jointCount > 0) || (morphCount > 0);

			// Just override it if we don't plan on animating it anyway
			if (!animComp)
				objectAnimated = false;

			params->drawIsAnimated.push_back(objectAnimated ? 1u : 0u);

			if (objectAnimated)
			{
				SceneRenderParams::AnimatedInstanceParams instance{};
				instance.drawIndex = drawCommandIndex;
				instance.srcBaseVertex = static_cast<uint32_t>(meshData->vertexByteOffset / sizeof(CompressedVertex));
				instance.dstBaseVertex = animatedVertexCursor;
				instance.vertexCount = meshData->vertexCount;
				instance.meshDecompIndex = drawData.meshDecompIndex;

				if (meshData->animation.skinningByteOffset != UINT32_MAX)
					instance.skinningOffset = meshData->animation.skinningByteOffset / sizeof(GPUSkinningData);

				if (animComp && animComp->jointCount > 0 && !animComp->skinMatrices.empty())
				{
					instance.boneMatrixOffset = static_cast<uint32_t>(params->boneMatrices.size());
					instance.jointCount = animComp->jointCount;
					params->boneMatrices.insert(params->boneMatrices.end(),
						animComp->skinMatrices.begin(),
						animComp->skinMatrices.begin() + animComp->jointCount);
				}

				if (meshData->animation.morphDeltaByteOffset != UINT32_MAX)
					instance.morphDeltaOffset = meshData->animation.morphDeltaByteOffset / sizeof(GPUMorphDelta);
				instance.morphDeltaCount = meshData->animation.morphDeltaCount;

				if (meshData->animation.morphVertexBaseOffset != UINT32_MAX)
					instance.morphVertexBaseOffset = meshData->animation.morphVertexBaseOffset / sizeof(uint32_t);
				if (meshData->animation.morphVertexCountOffset != UINT32_MAX)
					instance.morphVertexCountOffset = meshData->animation.morphVertexCountOffset / sizeof(uint32_t);

				if (animComp && animComp->morphCount > 0 && !animComp->morphWeights.empty())
				{
					instance.morphStateOffset = static_cast<uint32_t>(params->morphWeights.size());
					instance.morphTargetCount = animComp->morphCount;
					params->morphWeights.insert(params->morphWeights.end(),
						animComp->morphWeights.begin(),
						animComp->morphWeights.begin() + animComp->morphCount);
				}

				params->drawCommands[drawCommandIndex].baseVertex = static_cast<int32_t>(instance.dstBaseVertex);
				params->animatedInstances.push_back(instance);
				params->hasAnimatedInstances = true;
				animatedVertexCursor += instance.vertexCount;
			}

			// Sort into opaque/transparent queues
			if (context.resourceMngr->isMaterialTransparent(materialHandle))
			{
				params->transparentIndices.push_back(drawCommandIndex);
				if (objectAnimated)
					++params->transparentAnimatedCount;
				else
					++params->transparentStaticCount;
			}
			else
			{
				params->opaqueIndices.push_back(drawCommandIndex);
				if (objectAnimated)
					++params->opaqueAnimatedCount;
				else
					++params->opaqueStaticCount;
			}


			++objectIndex;
		}
	}

	if (params->animatedInstances.empty())
	{
		params->boneMatrices.clear();
		params->morphWeights.clear();
		params->hasAnimatedInstances = false;
	}
	else
	{
		params->hasAnimatedInstances = true;
	}


	params->usedAnimatedVertexBytes = animatedVertexCursor * sizeof(SkinnedVertex);


	// Iterate all LightComponents in the ECS and populate light data directly
	params->lights.clear();
	for (auto lightIter = ecs::GetCompsActiveBegin<LightComponent>(); lightIter != ecs::GetCompsEnd<LightComponent>(); ++lightIter)
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
		gpuLight.position = lightIter.GetEntity()->GetTransform().GetWorldPosition();
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

	// Disable sample 2D UI for now
	if (false && ui2dFeatureHandle != 0 && context.resourceMngr)
	{
		ui::ImmediateGui overlayGui(*context.renderer, ui2dFeatureHandle, *context.resourceMngr);
		FontHandle font = ui2dFontHandle;
		if (overlayGui.begin(font))
		{
			overlayGui.setViewport(width, height);

			// ==========================================================================
			// COMPREHENSIVE UI::IMMEDIATGUI API DEMONSTRATION
			// ==========================================================================
			// This demo showcases ALL features of the ui::ImmediateGui interface.
			// Each section is documented to teach junior developers how to use the API.
			// ==========================================================================

			// --------------------------------------------------------------------------
			// SECTION 1: BASIC RECTANGLES AND SHAPES
			// --------------------------------------------------------------------------
			// Solid rectangles are the foundation of UI panels and backgrounds.
			// They require: min corner (top-left), max corner (bottom-right), and color.

			const vec2 panel1Min{ 18.0f, 18.0f };
			const vec2 panel1Max{ panel1Min.x + 380.0f, panel1Min.y + 180.0f };

			ui::DrawOptions panelOptions;
			panelOptions.layer = 0; // Lower layers render first (background)

			// addSolidRect: Creates filled rectangular regions
			overlayGui.addSolidRect(panel1Min, panel1Max,
				vec4(0.05f, 0.07f, 0.1f, 0.85f), // RGBA color (semi-transparent dark blue)
				panelOptions);

			// addRectOutline: Creates rectangular borders/frames
			ui::DrawOptions borderOptions = panelOptions;
			borderOptions.layer = 1; // Draw border on top of background
			overlayGui.addRectOutline(panel1Min, panel1Max,
				1.5f, // Border thickness in pixels
				vec4(0.15f, 0.4f, 0.8f, 1.0f), // Bright blue, fully opaque
				borderOptions);

			// --------------------------------------------------------------------------
			// SECTION 2: TEXT RENDERING WITH LAYOUT CONTROL
			// --------------------------------------------------------------------------
			// Text rendering supports formatting, wrapping, clipping, and spacing.
			// TextLayoutDesc controls all text positioning and behavior.

			ui::TextLayoutDesc titleLayout;
			titleLayout.origin = panel1Min + vec2(16.0f, 34.0f); // Position from top-left
			titleLayout.clipRect = vec4(panel1Min.x, panel1Min.y, panel1Max.x, panel1Min.y + 52.0f);
			// Note: clipRect format is (minX, minY, maxX, maxY)

			// Technique: Drop shadow for enhanced text readability
			ui::DrawOptions titleShadowOptions;
			titleShadowOptions.layer = 1;
			titleShadowOptions.hasClipRect = true; // Enable clipping
			titleShadowOptions.clipRect = titleLayout.clipRect;

			ui::TextLayoutDesc shadowLayout = titleLayout;
			shadowLayout.origin += vec2(1.5f, 1.5f); // Offset shadow slightly down-right
			overlayGui.addText("UI API Feature Demo",
				shadowLayout,
				vec4(0.0f, 0.0f, 0.0f, 0.6f), // Semi-transparent black shadow
				titleShadowOptions);

			// Main title text on top of shadow
			ui::DrawOptions titleOptions = titleShadowOptions;
			titleOptions.layer = 2; // Render above shadow
			overlayGui.addText("UI API Feature Demo",
				titleLayout,
				vec4(0.95f, 0.96f, 0.98f, 1.0f), // Bright white text
				titleOptions);

			// --------------------------------------------------------------------------
			// SECTION 3: TEXT MEASUREMENT AND DYNAMIC LAYOUT
			// --------------------------------------------------------------------------
			// measureText: Calculate text dimensions before rendering for dynamic layouts

			const vec2 titleSize = overlayGui.measureText("UI API Feature Demo", titleLayout);

			// Use measured size to create a decorative underline
			const vec2 underlineMin = titleLayout.origin + vec2(-4.0f, titleSize.y + 4.0f);
			const vec2 underlineMax = vec2(underlineMin.x + titleSize.x + 8.0f, underlineMin.y + 3.0f);
			overlayGui.addSolidRect(underlineMin, underlineMax,
				vec4(0.2f, 0.55f, 0.95f, 0.55f), // Semi-transparent blue accent
				titleOptions);

			// --------------------------------------------------------------------------
			// SECTION 4: TEXT WRAPPING AND FORMATTING
			// --------------------------------------------------------------------------
			// wrapWidth: Enable automatic word wrapping within a specified width

			ui::TextLayoutDesc wrappingLayout;
			wrappingLayout.origin = panel1Min + vec2(16.0f, 64.0f);
			wrappingLayout.wrapWidth = 348.0f; // Panel width minus padding
			wrappingLayout.lineSpacing = 1.2f;

			// Measure wrapped text to determine required space
			const char* wrappingText = "Text wrapping demo: This paragraph demonstrates automatic word wrapping. "
				"The text will flow within the specified width and clip at panel boundaries.";
			vec2 wrappingSize = overlayGui.measureText(wrappingText, wrappingLayout);

			ui::DrawOptions wrappingOptions;
			wrappingOptions.layer = 2;
			wrappingOptions.hasClipRect = true;
			wrappingOptions.clipRect = vec4(panel1Min.x, panel1Min.y, panel1Max.x, panel1Max.y);

			overlayGui.addText(wrappingText,
				wrappingLayout,
				vec4(0.90f, 0.92f, 0.95f, 1.0f),
				wrappingOptions);

			// --------------------------------------------------------------------------
			// SECTION 5: FORMATTED TEXT WITH PARAMETERS
			// --------------------------------------------------------------------------
			// addText supports fmt::format-style string formatting for dynamic values

			// Create a separate panel for formatted text demo
			const vec2 panel1bMin{ 18.0f, panel1Max.y + 20.0f };

			ui::TextLayoutDesc fmtTitleLayout;
			fmtTitleLayout.origin = panel1bMin + vec2(16.0f, 28.0f);
			const vec2 fmtTitleSize = overlayGui.measureText("Formatted Text", fmtTitleLayout);

			uint32_t objectCount = static_cast<uint32_t>(params->objectTransforms.size());
			uint32_t lightCount = params->activeLightCount;

			ui::TextLayoutDesc statsLayout;
			statsLayout.origin = panel1bMin + vec2(16.0f, 28.0f + fmtTitleSize.y + 12.0f);
			statsLayout.wrapWidth = 348.0f;

			// Measure the formatted content
			char statsBuffer[128];
			std::snprintf(statsBuffer, sizeof(statsBuffer),
				"Objects: %u | Lights: %u | Frame: %u", objectCount, lightCount, 42);
			const vec2 statsSize = overlayGui.measureText(statsBuffer, statsLayout);

			// Calculate panel size based on content
			const vec2 panel1bMax{ panel1bMin.x + 380.0f,
								   panel1bMin.y + fmtTitleSize.y + statsSize.y + 60.0f };

			overlayGui.addSolidRect(panel1bMin, panel1bMax,
				vec4(0.05f, 0.07f, 0.1f, 0.85f), panelOptions);
			overlayGui.addRectOutline(panel1bMin, panel1bMax, 1.5f,
				vec4(0.15f, 0.4f, 0.8f, 1.0f), borderOptions);

			ui::DrawOptions fmtTitleOpts;
			fmtTitleOpts.layer = 2;
			overlayGui.addText("Formatted Text",
				fmtTitleLayout,
				vec4(0.95f, 0.96f, 0.98f, 1.0f),
				fmtTitleOpts);

			ui::DrawOptions statsOptions;
			statsOptions.layer = 2;
			overlayGui.addText(
				"Objects: {} | Lights: {} | Frame: {}",
				statsLayout,
				vec4(0.85f, 0.88f, 0.92f, 1.0f),
				statsOptions,
				objectCount, lightCount, 42); // Format arguments

			// --------------------------------------------------------------------------
			// SECTION 6: LAYERING AND COMPOSITING
			// --------------------------------------------------------------------------
			// Layers control draw order. Higher layer numbers render on top.
			// This section demonstrates a "badge" that overlays previous elements.

			ui::DrawOptions badgeOptions;
			badgeOptions.layer = 3; // Draw on top of everything else
			badgeOptions.hasClipRect = true;
			badgeOptions.clipRect = vec4(panel1Max.x - 150.0f, panel1Max.y - 54.0f,
				panel1Max.x - 12.0f, panel1Max.y - 12.0f);

			const vec2 badgeMin = vec2(badgeOptions.clipRect.x, badgeOptions.clipRect.y);
			const vec2 badgeMax = vec2(badgeOptions.clipRect.z, badgeOptions.clipRect.w);

			overlayGui.addSolidRect(badgeMin, badgeMax,
				vec4(0.08f, 0.26f, 0.4f, 0.92f), // Dark cyan badge background
				badgeOptions);

			ui::TextLayoutDesc badgeLayout;
			badgeLayout.origin = badgeMin + vec2(14.0f, 14.0f);
			badgeLayout.clipRect = badgeOptions.clipRect;
			badgeLayout.lineSpacing = 1.0f;

			overlayGui.addText("Layer {}\nDemo",
				badgeLayout,
				vec4(0.9f, 0.95f, 1.0f, 1.0f),
				badgeOptions,
				badgeOptions.layer); // Pass layer number as format arg

			// --------------------------------------------------------------------------
			// SECTION 7: LINES AND POLYLINES
			// --------------------------------------------------------------------------
			// Lines connect two points. Polylines connect multiple points.

			const vec2 panel2Min{ panel1Max.x + 20.0f, 18.0f };

			// Measure title to help size the panel
			ui::TextLayoutDesc shapesTitleLayout;
			shapesTitleLayout.origin = panel2Min + vec2(16.0f, 28.0f);
			const vec2 shapesTitleSize = overlayGui.measureText("Shapes & Lines", shapesTitleLayout);

			// Panel needs space for title + shapes (estimated)
			const vec2 panel2Max{ panel2Min.x + 280.0f, panel2Min.y + 200.0f };

			// Background panel for shape demos
			overlayGui.addSolidRect(panel2Min, panel2Max,
				vec4(0.04f, 0.06f, 0.09f, 0.85f), panelOptions);
			overlayGui.addRectOutline(panel2Min, panel2Max, 1.5f,
				vec4(0.15f, 0.4f, 0.8f, 1.0f), borderOptions);

			// Title for shapes panel
			ui::DrawOptions shapesTitleOpts;
			shapesTitleOpts.layer = 2;
			overlayGui.addText("Shapes & Lines",
				shapesTitleLayout,
				vec4(0.95f, 0.96f, 0.98f, 1.0f),
				shapesTitleOpts);

			// Simple line
			const vec2 lineStart = panel2Min + vec2(20.0f, 60.0f);
			const vec2 lineEnd = panel2Min + vec2(120.0f, 60.0f);
			overlayGui.addLine(lineStart, lineEnd,
				vec4(0.3f, 0.8f, 0.4f, 1.0f), // Green line
				2.0f); // Thickness in pixels

			// Polyline (connected line segments)
			vec2 polyPoints[] = {
				panel2Min + vec2(140.0f, 50.0f),
				panel2Min + vec2(180.0f, 70.0f),
				panel2Min + vec2(220.0f, 50.0f),
				panel2Min + vec2(260.0f, 70.0f)
			};
			overlayGui.addPolyline(polyPoints, 4,
				false, // closed = false (open polyline)
				vec4(0.9f, 0.5f, 0.2f, 1.0f), // Orange
				2.0f);

			// --------------------------------------------------------------------------
			// SECTION 8: TRIANGLES (OUTLINED AND FILLED)
			// --------------------------------------------------------------------------

			vec2 tri1p1 = panel2Min + vec2(30.0f, 90.0f);
			vec2 tri1p2 = panel2Min + vec2(60.0f, 90.0f);
			vec2 tri1p3 = panel2Min + vec2(45.0f, 120.0f);

			// Outlined triangle
			overlayGui.addTriangle(tri1p1, tri1p2, tri1p3,
				vec4(0.8f, 0.3f, 0.3f, 1.0f), // Red outline
				2.0f);

			// Filled triangle
			vec2 tri2p1 = panel2Min + vec2(80.0f, 90.0f);
			vec2 tri2p2 = panel2Min + vec2(110.0f, 90.0f);
			vec2 tri2p3 = panel2Min + vec2(95.0f, 120.0f);
			overlayGui.addTriangleFilled(tri2p1, tri2p2, tri2p3,
				vec4(0.3f, 0.3f, 0.8f, 0.7f)); // Semi-transparent blue fill

			// --------------------------------------------------------------------------
			// SECTION 9: CIRCLES (OUTLINED AND FILLED)
			// --------------------------------------------------------------------------
			// numSegments: Controls circle smoothness (0 = auto-calculate, 12-64 typical)

			vec2 circleCenter1 = panel2Min + vec2(150.0f, 105.0f);
			overlayGui.addCircle(circleCenter1,
				15.0f, // Radius
				vec4(0.9f, 0.7f, 0.2f, 1.0f), // Yellow outline
				24, // 24 segments for smooth circle
				2.0f); // Line thickness

			vec2 circleCenter2 = panel2Min + vec2(200.0f, 105.0f);
			overlayGui.addCircleFilled(circleCenter2,
				15.0f,
				vec4(0.2f, 0.7f, 0.5f, 0.8f), // Teal fill
				24); // numSegments

			// --------------------------------------------------------------------------
			// SECTION 10: N-GONS (REGULAR POLYGONS)
			// --------------------------------------------------------------------------
			// N-gons are regular polygons with N sides (pentagon, hexagon, etc.)

			vec2 ngonCenter1 = panel2Min + vec2(35.0f, 150.0f);
			overlayGui.addNgon(ngonCenter1,
				12.0f, // Radius
				5, // 5 sides = pentagon
				vec4(0.8f, 0.4f, 0.8f, 1.0f), // Magenta outline
				2.0f); // Thickness

			vec2 ngonCenter2 = panel2Min + vec2(80.0f, 150.0f);
			overlayGui.addNgonFilled(ngonCenter2,
				12.0f,
				6, // 6 sides = hexagon
				vec4(0.4f, 0.6f, 0.9f, 0.8f)); // Light blue fill

			vec2 ngonCenter3 = panel2Min + vec2(125.0f, 150.0f);
			overlayGui.addNgonFilled(ngonCenter3,
				12.0f,
				8, // 8 sides = octagon
				vec4(0.9f, 0.6f, 0.3f, 0.8f)); // Orange fill

			// --------------------------------------------------------------------------
			// SECTION 11: ADVANCED CLIPPING
			// --------------------------------------------------------------------------
			// Clipping restricts rendering to a specific rectangular region.
			// Content outside the clip rect is automatically culled.

			const vec2 panel3Min{ 18.0f, panel1bMax.y + 20.0f };

			// Measure title and content
			ui::TextLayoutDesc clipTitleLayout;
			clipTitleLayout.origin = panel3Min + vec2(16.0f, 28.0f);
			const vec2 clipTitleSize = overlayGui.measureText("Advanced Clipping & Options", clipTitleLayout);

			// Panel sized for content + demo elements
			const vec2 panel3Max{ panel3Min.x + 380.0f, panel3Min.y + 160.0f };

			overlayGui.addSolidRect(panel3Min, panel3Max,
				vec4(0.06f, 0.05f, 0.08f, 0.85f), panelOptions);
			overlayGui.addRectOutline(panel3Min, panel3Max, 1.5f,
				vec4(0.6f, 0.2f, 0.7f, 1.0f), // Purple border
				borderOptions);

			ui::DrawOptions clipTitleOpts;
			clipTitleOpts.layer = 2;
			overlayGui.addText("Advanced Clipping & Options",
				clipTitleLayout,
				vec4(0.95f, 0.96f, 0.98f, 1.0f),
				clipTitleOpts);

			// Demonstrate clipping with overlapping rectangles
			ui::DrawOptions clipDemo;
			clipDemo.layer = 1;
			clipDemo.hasClipRect = true;
			clipDemo.clipRect = vec4(panel3Min.x + 20.0f, panel3Min.y + 50.0f,
				panel3Min.x + 200.0f, panel3Min.y + 120.0f);

			// This rectangle extends beyond clip region (will be clipped)
			overlayGui.addSolidRect(
				panel3Min + vec2(10.0f, 40.0f),
				panel3Min + vec2(220.0f, 130.0f),
				vec4(0.8f, 0.2f, 0.2f, 0.5f),
				clipDemo);

			// Visual indicator of clip boundary
			overlayGui.addRectOutline(
				vec2(clipDemo.clipRect.x, clipDemo.clipRect.y),
				vec2(clipDemo.clipRect.z, clipDemo.clipRect.w),
				2.0f,
				vec4(1.0f, 1.0f, 0.0f, 0.8f), // Yellow outline showing clip rect
				borderOptions);

			// --------------------------------------------------------------------------
			// SECTION 12: LETTER SPACING AND LINE SPACING
			// --------------------------------------------------------------------------
			// Fine-tune text appearance with spacing controls

			ui::TextLayoutDesc spacingDemo;
			spacingDemo.origin = panel3Min + vec2(220.0f, 58.0f);
			spacingDemo.wrapWidth = 140.0f;
			spacingDemo.letterSpacing = 2.0f; // Extra space between characters
			spacingDemo.lineSpacing = 1.5f; // 50% extra line height

			ui::DrawOptions spacingTextOpts;
			spacingTextOpts.layer = 2;
			overlayGui.addText(
				"Letter & Line\nSpacing Demo",
				spacingDemo,
				vec4(0.85f, 0.88f, 0.92f, 1.0f),
				spacingTextOpts);

			// --------------------------------------------------------------------------
			// SECTION 13: CLOSED POLYLINES (POLYGONS)
			// --------------------------------------------------------------------------
			// closed = true creates an enclosed shape from connected line segments

			vec2 polygonPoints[] = {
				panel3Min + vec2(30.0f, 95.0f),
				panel3Min + vec2(70.0f, 85.0f),
				panel3Min + vec2(90.0f, 110.0f),
				panel3Min + vec2(60.0f, 125.0f),
				panel3Min + vec2(20.0f, 115.0f)
			};
			overlayGui.addPolyline(polygonPoints, 5,
				true, // closed = true (connect last point to first)
				vec4(0.3f, 0.9f, 0.8f, 1.0f), // Cyan
				2.0f);

			// Label for polygon
			ui::TextLayoutDesc polyLabel;
			polyLabel.origin = panel3Min + vec2(25.0f, 132.0f);

			ui::DrawOptions polyLabelOpts;
			polyLabelOpts.layer = 2;
			overlayGui.addText("Closed Polyline",
				polyLabel,
				vec4(0.7f, 0.7f, 0.7f, 1.0f),
				polyLabelOpts);

			// --------------------------------------------------------------------------
			// SECTION 14: PIXEL-PERFECT RENDERING OPTIONS
			// --------------------------------------------------------------------------
			// snapToPixel: Align text to pixel boundaries for crisp rendering
			// originIsBaseline: Control whether origin is top or baseline of text

			ui::TextLayoutDesc pixelPerfect;
			pixelPerfect.origin = panel3Min + vec2(120.0f, 100.0f);
			pixelPerfect.snapToPixel = true; // Align to pixel grid
			pixelPerfect.originIsBaseline = false; // Origin at top-left
			pixelPerfect.wrapWidth = 80.0f;

			ui::DrawOptions pixelPerfectOpts;
			pixelPerfectOpts.layer = 2;
			overlayGui.addText("Pixel-Snapped\nText Rendering",
				pixelPerfect,
				vec4(0.9f, 0.9f, 0.5f, 1.0f),
				pixelPerfectOpts);

			// --------------------------------------------------------------------------
			// SECTION 15: MULTI-LAYER COMPOSITION
			// --------------------------------------------------------------------------
			// Demonstrate complex layering with multiple overlapping elements

			const vec2 panel4Min{ panel2Min.x, panel2Max.y + 20.0f };

			// Measure title and labels
			ui::TextLayoutDesc layerTitleLayout;
			layerTitleLayout.origin = panel4Min + vec2(16.0f, 28.0f);
			const vec2 layerTitleSize = overlayGui.measureText("Layer Composition", layerTitleLayout);

			ui::TextLayoutDesc layerLabelsLayout;
			layerLabelsLayout.origin = panel4Min + vec2(20.0f, 115.0f);
			layerLabelsLayout.lineSpacing = 1.3f;
			const vec2 labelsSize = overlayGui.measureText(
				"Layer 0: Circles (back)\nLayer 1: Triangles (mid)\nLayer 2: Rects (front)",
				layerLabelsLayout);

			// Size panel to fit all content
			const vec2 panel4Max{ panel4Min.x + 280.0f,
								  panel4Min.y + layerTitleSize.y + 115.0f + labelsSize.y + 20.0f };

			overlayGui.addSolidRect(panel4Min, panel4Max,
				vec4(0.05f, 0.08f, 0.06f, 0.85f), panelOptions);
			overlayGui.addRectOutline(panel4Min, panel4Max, 1.5f,
				vec4(0.2f, 0.7f, 0.3f, 1.0f), // Green border
				borderOptions);

			ui::DrawOptions layerTitleOpts;
			layerTitleOpts.layer = 3;
			overlayGui.addText("Layer Composition",
				layerTitleLayout,
				vec4(0.95f, 0.96f, 0.98f, 1.0f),
				layerTitleOpts);

			// Layer 0: Background circles
			for (int i = 0; i < 3; ++i)
			{
				ui::DrawOptions layer0;
				layer0.layer = 0;
				vec2 center = panel4Min + vec2(60.0f + i * 50.0f, 80.0f);
				overlayGui.addCircleFilled(center, 20.0f,
					vec4(0.2f, 0.3f, 0.4f, 0.6f), 24, layer0);
			}

			// Layer 1: Middle triangles
			for (int i = 0; i < 3; ++i)
			{
				ui::DrawOptions layer1;
				layer1.layer = 1;
				vec2 offset = panel4Min + vec2(50.0f + i * 50.0f, 75.0f);
				overlayGui.addTriangleFilled(
					offset + vec2(0.0f, 0.0f),
					offset + vec2(30.0f, 0.0f),
					offset + vec2(15.0f, 25.0f),
					vec4(0.5f, 0.6f, 0.2f, 0.7f),
					layer1);
			}

			// Layer 2: Top rectangles
			for (int i = 0; i < 3; ++i)
			{
				ui::DrawOptions layer2;
				layer2.layer = 2;
				vec2 rectMin = panel4Min + vec2(55.0f + i * 50.0f, 85.0f);
				overlayGui.addSolidRect(rectMin, rectMin + vec2(20.0f, 20.0f),
					vec4(0.8f, 0.3f, 0.4f, 0.8f), layer2);
			}

			// Labels for layers (reuse measured layout)
			ui::DrawOptions layerLabelsOpts;
			layerLabelsOpts.layer = 3;
			overlayGui.addText(
				"Layer 0: Circles (back)\nLayer 1: Triangles (mid)\nLayer 2: Rects (front)",
				layerLabelsLayout,
				vec4(0.85f, 0.85f, 0.85f, 1.0f),
				layerLabelsOpts);

			// --------------------------------------------------------------------------
			// SECTION 16: DRAW OPTIONS SUMMARY REFERENCE
			// --------------------------------------------------------------------------
			// Quick reference panel showing all DrawOptions fields

			const vec2 refMin{ 18.0f, panel3Max.y + 20.0f };

			// Measure title
			ui::TextLayoutDesc refTitleLayout;
			refTitleLayout.origin = refMin + vec2(16.0f, 28.0f);
			const vec2 refTitleSize = overlayGui.measureText("API Quick Reference", refTitleLayout);

			// Measure content with wrapping
			ui::TextLayoutDesc refContentLayout;
			refContentLayout.origin = refMin + vec2(16.0f, 28.0f + refTitleSize.y + 16.0f);
			refContentLayout.lineSpacing = 1.25f;
			refContentLayout.wrapWidth = 646.0f; // Panel width - padding

			const char* refText =
				"DrawOptions: layer (0-65535), hasClipRect (bool), clipRect (vec4), fontOverride (FontHandle)\n"
				"TextLayoutDesc: origin, clipRect, lineSpacing, letterSpacing, wrapWidth, snapToPixel, originIsBaseline\n"
				"Colors: vec4(R, G, B, A) - values 0.0 to 1.0, alpha controls transparency\n"
				"Layers: Lower numbers render first. Use for z-ordering UI elements.";
			const vec2 refContentSize = overlayGui.measureText(refText, refContentLayout);

			// Calculate panel size based on measured content
			const vec2 refMax{ refMin.x + 678.0f,
							   refMin.y + refTitleSize.y + refContentSize.y + 60.0f };

			overlayGui.addSolidRect(refMin, refMax,
				vec4(0.08f, 0.06f, 0.04f, 0.85f), panelOptions);
			overlayGui.addRectOutline(refMin, refMax, 1.5f,
				vec4(0.8f, 0.6f, 0.2f, 1.0f), // Gold border
				borderOptions);

			ui::DrawOptions refTitleOpts;
			refTitleOpts.layer = 2;
			overlayGui.addText("API Quick Reference",
				refTitleLayout,
				vec4(1.0f, 0.9f, 0.6f, 1.0f),
				refTitleOpts);

			ui::DrawOptions refContentOpts;
			refContentOpts.layer = 2;
			overlayGui.addText(refText,
				refContentLayout,
				vec4(0.9f, 0.85f, 0.7f, 1.0f),
				refContentOpts);
		}
		overlayGui.end();
	}
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
	const std::string physicalFilepath{ VFS::ConvertVirtualToPhysical(fontfile) };

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
	io.Fonts->AddFontFromFileTTF(physicalFilepath.c_str(), baseFontSize);
	io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, iconFontSize, &icons_config, icons_ranges);

	imguiContext->rebuildFontAtlas();
	//If you want change between icons size you will need to create a new font
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 12.0f, &icons_config, icons_ranges);
	//io.Fonts->AddFontFromMemoryCompressedTTF(FA_compressed_data, FA_compressed_size, 20.0f, &icons_config, icons_ranges);

	// Create a separate font for UI2D immediate mode rendering
	size_t dataSize = 0;
	void* rawData = ImFileLoadToMemory(physicalFilepath.c_str(), "rb", &dataSize, 0);
	if (rawData && dataSize > 0)
	{
		std::vector fontBytes(static_cast<uint8_t*>(rawData), static_cast<uint8_t*>(rawData) + dataSize);
		IM_FREE(rawData);

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
	else if (rawData)
	{
		IM_FREE(rawData);
	}
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
