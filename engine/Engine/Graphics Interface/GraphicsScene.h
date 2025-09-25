#pragma once
#include "context.h"
#include "scene_feature.h"
#include "asset_types.h"

class GraphicsScene
{
public:
	~GraphicsScene();

	bool Init(Context inContext);
	bool NewFrame();

	void AddObject(const MeshHandle& meshHandle, const MaterialHandle& materialHandle, const Mat4& transform);

private:
	friend ST<GraphicsScene>;
	GraphicsScene();

private:
	Context context;
	uint64_t sceneFeatureHandle;
	SceneRenderParams* params;

	size_t objIndex;

	uint64_t gridHandle;
};

