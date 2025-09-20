#include "GraphicsScene.h"
#include "renderer.h"
#include "grid_feature.h"
#include "asset_system.h"

GraphicsScene::GraphicsScene()
    : context{}
    , sceneFeatureHandle{}
    , params{}
    , objIndex{}
    , gridHandle{}
{
}

GraphicsScene::~GraphicsScene()
{
    if (!context.renderer)
        return;

    context.renderer->DestroyFeature(gridHandle);
    context.renderer->DestroyFeature(sceneFeatureHandle);
}

bool GraphicsScene::Init(Context inContext)
{
    context = inContext;
    sceneFeatureHandle = context.renderer->CreateFeature<SceneRenderFeature>();
    gridHandle = context.renderer->CreateFeature<GridFeature>();
    return true;
}

bool GraphicsScene::NewFrame()
{
    params = static_cast<SceneRenderParams*>(context.renderer->GetFeatureParameterBlockPtr(sceneFeatureHandle));
    if (!params)
        return false;

    params->clear();
    objIndex = 0;
}

void GraphicsScene::AddObject(const ResourceMesh* resource, const Mat4& transform)
{
    for (const auto& obj : resource->objects)
    {
        INTERNAL_AddSceneObject(obj, transform);
        ++objIndex;
    }
}

void GraphicsScene::INTERNAL_AddSceneObject(const SceneObject& object, const Mat4& transform)
{
    // Validate handles
    if (!object.mesh.isValid() || !object.material.isValid())
        return;

    const auto* meshData{ context.assetSystem->getMesh(object.mesh) };
    if (!meshData)
        return;

    // Store object data
    Mat4 finalTransform{ transform * object.transform };
    params->objectTransforms.push_back(finalTransform);
    params->materialIndices.push_back(context.assetSystem->getMaterialIndex(object.material));

    // Transform mesh bounds to world space
    Vec3 center{ meshData->bounds };
    Vec3 radius{ meshData->bounds.w };
    Vec3 worldCenter = Vec3(finalTransform * Vec4{ center, 1.0f });
    params->objectBounds.emplace_back(worldCenter - radius, worldCenter + radius);

    // Build draw command
    DrawIndexedIndirectCommand cmd{
        .count = meshData->indexCount,
        .instanceCount = 1,
        .firstIndex = static_cast<uint32_t>(meshData->indexByteOffset / sizeof(uint32_t)),
        .baseVertex = static_cast<int32_t>(meshData->vertexByteOffset / sizeof(Vertex)),
        .baseInstance = static_cast<uint32_t>(objIndex)
    };

    DrawData drawData{
        .transformId = static_cast<uint32_t>(objIndex),
        .materialId = context.assetSystem->getMaterialIndex(object.material)
    };

    uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.size());
    params->drawCommands.push_back(cmd);
    params->drawData.push_back(drawData);

    // Determine transparency and add to render queues
    if (context.assetSystem->isMaterialTransparent(object.material))
        params->transparentIndices.push_back(drawCommandIndex);
    else
        params->opaqueIndices.push_back(drawCommandIndex);
}
