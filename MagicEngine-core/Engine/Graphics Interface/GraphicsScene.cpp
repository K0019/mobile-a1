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
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "graphics/animation_update.h"

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

void GraphicsScene::AddAnimatedObject(const MeshHandle& meshHandle, const MaterialHandle& materialHandle, const Transform& transform, const Mat4& meshTransform, SceneObject::AnimBinding animBinding)
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

    // Animation info
    auto& graphicsAssetSystem = ST<GraphicsMain>::Get()->GetAssetSystem();
    const auto* meshMetadata = graphicsAssetSystem.getMeshMetadata(meshHandle);
    if (!meshMetadata)
        return;

    const bool hasSkeleton = meshMetadata->skeletonId != Resource::INVALID_SKELETON_ID;
    const bool hasMorphs = meshMetadata->morphSetId != Resource::INVALID_MORPH_SET_ID;

    if (!hasSkeleton && !hasMorphs)
        return;

    animBinding.skeleton = meshMetadata->skeletonId;
    animBinding.morphSet = meshMetadata->morphSetId;
    //Rest of animBinding params populated in AnimatorSystem

    if (hasSkeleton)
    {
        const Resource::Skeleton& skeleton = graphicsAssetSystem.Skeleton(meshMetadata->skeletonId);
        const uint32_t jointCount = skeleton.jointCount();
        animBinding.jointCount = static_cast<uint16_t>(std::min<uint32_t>(jointCount, std::numeric_limits<uint16_t>::max()));
        animBinding.skinMatrices.assign(animBinding.jointCount, glm::mat4(1.0f));
    }

    if (hasMorphs)
    {
        const Resource::MorphSet & morphSet = graphicsAssetSystem.Morph(meshMetadata->morphSetId);
        const uint32_t morphCount = morphSet.count();
        animBinding.morphCount = static_cast<uint16_t>(std::min<uint32_t>(morphCount, std::numeric_limits<uint16_t>::max()));
        animBinding.morphWeights.assign(animBinding.morphCount, 0.0f);
    }

    // Store mesh-specific data required for correct skinning
    const auto* meshCold = meshMetadata;
    animBinding.invBindMatrices = meshCold->jointInverseBindMatrices;
    animBinding.jointRemap.assign(animBinding.jointCount, -1);

    const auto& skeleton = graphicsAssetSystem.Skeleton(animBinding.skeleton);

    // Build the remap table: mesh joint index -> skeleton joint index
    for (uint16_t jMesh = 0; jMesh < animBinding.jointCount; ++jMesh)
    {
        const std::string& meshJointName = meshCold->jointNames[jMesh];
        const int16_t jSkel = skeleton.indexOfJoint(meshJointName);
        animBinding.jointRemap[jMesh] = jSkel;
    }


    newObj.anim = std::move(animBinding);

    Animation::Animate(*context.resourceMngr, newObj, 0.001f);
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
