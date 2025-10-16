/******************************************************************************/
/*!
\file   GraphicsScene.h
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

#pragma once
#include "context.h"
#include "scene_feature.h"
#include "asset_types.h"
#include "camera.h"
#include "EditorCameraBridge.h"
class GraphicsScene
{
public:
	~GraphicsScene();

	bool Init(Context inContext);
	bool NewFrame();

	void SetViewCamera(const Camera& camera);
	void AddObject(const MeshHandle& meshHandle, const MaterialHandle& materialHandle, const Mat4& transform);
	void AddLight(const SceneLight& sceneLight);

public:
	FrameData& INTERNAL_GetFrameData();

private:
	friend ST<GraphicsScene>;
	GraphicsScene();

private:
	Context context;
	uint64_t sceneFeatureHandle;
	SceneRenderParams* params;

	size_t objIndex;
	FrameData frameData;

	uint64_t gridHandle;
};

