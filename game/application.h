#pragma once
#include "Engine/Engine.h"
#include "renderer/frame_data.h"

struct Context;
struct RenderFrameData;

class GameApplication
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

	// Feature handles - Game only gets essential features (no grid)
	uint64_t sceneFeatureHandle_ = 0;
	uint64_t im3dFeatureHandle_ = 0;
	uint64_t ui2dFeatureHandle_ = 0;
};
