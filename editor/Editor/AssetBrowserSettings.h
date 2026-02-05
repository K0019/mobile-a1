#pragma once

namespace editor {

	// View settings shared across asset browser tabs
	// Split from AssetBrowser to avoid circular dependencies
	struct AssetBrowserSettings
	{
		enum class ViewMode {
			Grid,
			List
		};

		static float thumbnailSize;
		static ViewMode viewMode;
		static constexpr float MIN_THUMBNAIL_SIZE = 32.0f;
		static constexpr float MAX_THUMBNAIL_SIZE = 128.0f;
	};

}
