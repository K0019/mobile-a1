#pragma once
#include "Engine/Engine.h"
#include "renderer/frame_data.h"
#include "editor_viewport.h"

struct Context;
struct RenderFrameData;

class Application
{
public:
	void Initialize(Context& context);
	void Update(Context& context, RenderFrameData& frame);
	void Shutdown(Context& context);

private:
	void InitializeFeatures(Context& context);
	void DestroyFeatures(Context& context);
	void updateViews(Context& context, RenderFrameData& frame, int width, int height);

	MagicEngine magicEngine;
	editor::EditorViewport viewport_;

	// Feature handles - owned by the Application
	uint64_t sceneFeatureHandle_ = 0;
	uint64_t gridFeatureHandle_ = 0;
	uint64_t ui2dFeatureHandle_ = 0;
	uint64_t imguiFeatureHandle_ = 0;  // ImGui render feature handle
};
