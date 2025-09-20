#pragma once
#include "ResourceManager.h"
#include "context.h"
#include "scene_feature.h"

class GraphicsScene
{
public:
	GraphicsScene();

	bool Init(Context inContext);
	bool NewFrame();

	void AddObject(const ResourceMesh* resource, const Mat4& transform);

private:
	void INTERNAL_AddSceneObject(const SceneObject& object, const Mat4& transform);

private:
	Context context;
	uint64_t sceneFeatureHandle;
	SceneRenderParams* params;

	size_t objIndex;
};

