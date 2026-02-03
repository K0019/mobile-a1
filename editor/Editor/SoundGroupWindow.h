#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Assets/AssetHandle.h"
#include "Assets/Types/AssetTypesAudio.h"

namespace editor {

	class SoundGroupWindow : public WindowBase<SoundGroupWindow, false>
	{
	public:
		SoundGroupWindow();

		void DrawWindow() override;

	private:
		void DrawGroupsList();
		void DrawDetails();

	private:
		gui::TextBoxWithBuffer<64> groupFilename;
		std::vector<AssetHandle<ResourceAudio>> sounds;

	};

}
