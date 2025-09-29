#pragma once
#include "AssetBrowserCategories.h"

struct FrameData; //Forward declaration

struct AnimationTab
	: editor::BaseAssetCategory
{
	struct AnimationCreateConfig
	{
		bool showDialog = false; /**< Flag to show the create animation dialog */
		std::string animationName; /**< Name of the animation */
		std::vector<FrameData> frames; /**< Frames of the animation */
		bool isPlaying = false; /**< Flag indicating if the animation is playing */
		float previewTime = 0.0f; /**< Preview time for the animation */
		size_t currentFrame = 0; /**< Current frame of the animation */
		char spriteSearchBuffer[256] = ""; /**< Buffer for sprite search */
		std::string warningMessage; /**< Current warning message */
		bool hasWarning = false; /**< Flag indicating if there is a warning */

		/**
		 * @brief Reset the animation create configuration.
		 */
		void Reset();
	};

	AnimationCreateConfig animConfig;

	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
#ifdef IMGUI_ENABLED
	void ShowCreateAnimationDialog();
	void RenderSpriteSelectionGrid();
	void RenderFrameList();
	void RenderAnimationPreview();
	void RenderCreateAnimationBottom();
#endif

	float lastFrameTime = 0.0f; /**< Time of the last frame */
	float accumulatedTime = 0.0f; /**< Accumulated time for animation preview */
};
