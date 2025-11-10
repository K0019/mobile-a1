/******************************************************************************/
/*!
\file   GraphicsScene.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Abstracts the interface to upload objects into the render pipeline for rendering.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Graphics Interface/GraphicsScene.h"
#include "graphics/renderer.h"
#include "graphics/features/grid_feature.h"
#include "graphics/features/im3d_feature.h"
#include "resource/resource_manager.h"
#include "Editor/EditorCameraBridge.h"

GraphicsScene::GraphicsScene()
    : context{}
    , sceneFeatureHandle{}
    , gridHandle{}
{
}

GraphicsScene::~GraphicsScene()
{
    if (!context.renderer)
        return;

    context.renderer->DestroyFeature(gridHandle);
    context.renderer->DestroyFeature(sceneFeatureHandle);
	context.renderer->DestroyFeature(im3dHandle);
}

bool GraphicsScene::Init(Context inContext)
{
    context = inContext;
    sceneFeatureHandle = context.renderer->CreateFeature<SceneRenderFeature>();
    gridHandle = context.renderer->CreateFeature<GridFeature>();
	im3dHandle = context.renderer->CreateFeature<Im3dRenderFeature>();
    return true;
}

void GraphicsScene::Clear()
{
    scene.objects.clear();
    scene.lights.clear();
}

void GraphicsScene::UploadToPipeline(FrameData* outFrameData)
{
    SceneRenderFeature::UpdateScene(sceneFeatureHandle, scene, *context.resourceMngr, *context.renderer);

    if (auto params = static_cast<SceneRenderParams*>(context.renderer->GetFeatureParameterBlockPtr(sceneFeatureHandle)))
    {
        params->irradianceTexture = 0;
        params->prefilterTexture = 0;
        params->brdfLUT = 0;
        params->environmentIntensity = 1.0f;
    }

    float width{ static_cast<float>(Core::Display().GetWidth()) };
    float height{ static_cast<float>(Core::Display().GetHeight()) };
    outFrameData->cameraPos = frameData.cameraPos;
    outFrameData->viewMatrix = frameData.viewMatrix;
    outFrameData->projMatrix = glm::perspective(45.0f, width / height, 0.1f, 1000.0f);

    EditorCam_Publish(outFrameData->viewMatrix, outFrameData->projMatrix, false);
}

void GraphicsScene::SetViewCamera(const Camera& camera)
{
    frameData.cameraPos = camera.getPosition();
    frameData.viewMatrix = camera.getViewMatrix();
}

void GraphicsScene::AddObject(const MeshHandle& meshHandle, const MaterialHandle& materialHandle, const Transform& transform, const Mat4& meshTransform)
{
    // Validate handles
    if (!meshHandle.isValid() || !materialHandle.isValid())
        return;

    const auto* meshData{ context.resourceMngr->getMesh(meshHandle) };
    if (!meshData)
        return;

    // Store object data
    auto& newObj{ scene.objects.emplace_back() };
    newObj.type = SceneObjectType::Mesh;
    newObj.mesh = meshHandle;
    newObj.material = materialHandle;
    newObj.transform = transform.GetWorldMat() * meshTransform;
    newObj.maxScale = glm::compMax(glm::abs(static_cast<glm::vec3>(transform.GetWorldScale())));
}

void GraphicsScene::AddLight(const SceneLight& sceneLight)
{
    // Skip disabled lights
    if (sceneLight.intensity <= 0.0f)
        return;

    // Skip lights with zero/invalid color
    if (length(sceneLight.color) <= 0.0f)
        return;

    scene.lights.push_back(sceneLight);
}

FrameData& GraphicsScene::INTERNAL_GetFrameData()
{
    return frameData;
}
