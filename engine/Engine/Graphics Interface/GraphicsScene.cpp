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

void GraphicsScene::AddObject(const MeshHandle& meshHandle, const MaterialHandle& materialHandle, const Mat4& transform)
{
    // Validate handles
    if (!meshHandle.isValid() || !materialHandle.isValid())
        return;

    const auto* meshData{ context.assetSystem->getMesh(meshHandle) };
    if (!meshData)
        return;

    // Store object data
    params->objectTransforms.push_back(transform);
    params->materialIndices.push_back(context.assetSystem->getMaterialIndex(materialHandle));

    // Transform mesh bounds to world space
    Vec3 center{ meshData->bounds };
    Vec3 radius{ meshData->bounds.w };
    Vec3 worldCenter = Vec3(transform * Vec4{ center, 1.0f });
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
        .materialId = context.assetSystem->getMaterialIndex(materialHandle)
    };

    uint32_t drawCommandIndex = static_cast<uint32_t>(params->drawCommands.size());
    params->drawCommands.push_back(cmd);
    params->drawData.push_back(drawData);

    // Determine transparency and add to render queues
    if (context.assetSystem->isMaterialTransparent(materialHandle))
        params->transparentIndices.push_back(drawCommandIndex);
    else
        params->opaqueIndices.push_back(drawCommandIndex);

    // Track next object's index
    ++objIndex;
}
