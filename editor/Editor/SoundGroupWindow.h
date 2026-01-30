#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Resources/ResourcesHeader.h"
#include "Resources/Types/ResourceTypesAudio.h"

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
		std::vector<UserResourceHandle<ResourceAudio>> sounds;

	};

}
