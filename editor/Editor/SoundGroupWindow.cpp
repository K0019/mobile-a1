#include "Editor/SoundGroupWindow.h"
#include "VFS/VFS.h"
#include "Events/EventsQueue.h"
#include "Events/EventsTypeEditor.h"
#include "FilepathConstants.h"
#include "Engine/Resources/ResourceImporter.h"
#include "Editor/EditorUtilResource.h"

namespace editor {

	SoundGroupWindow::SoundGroupWindow()
		: WindowBase{ "Sound Groups", gui::Vec2{ 600, 600 } }
		, groupFilename{ "Filename" }
	{
	}

	void SoundGroupWindow::DrawWindow()
	{
		DrawGroupsList();
		gui::SameLine();
		DrawDetails();
	}

	void SoundGroupWindow::DrawGroupsList()
	{
		gui::Child soundGroupsListChild{ "SoundGroupsList", gui::Vec2{ 200.0f, 0.0f }, gui::FLAG_CHILD::BORDERS };

		gui::TextCenteredUnformatted("Sound Groups");
		gui::Separator();

		for (const auto& soundGroup : ST<MagicResourceManager>::Get()->Editor_GetContainer<ResourceAudioGroup>().Editor_GetAllResources())
		{
			const std::string* soundGroupName{ ST<MagicResourceManager>::Get()->Editor_GetName(soundGroup.first) };
			if (gui::Selectable(soundGroupName->c_str()))
			{
				UserResourceHandle<ResourceAudioGroup> soundGroupResource{ soundGroup.first };
				groupFilename.SetBuffer(soundGroupName->substr(0, soundGroupName->size() - 1));

				const auto& loadedSoundGroupSounds{ soundGroupResource.GetResource()->audio };
				sounds.clear();
				std::transform(loadedSoundGroupSounds.begin(), loadedSoundGroupSounds.end(), std::back_inserter(sounds), [](size_t hash) -> UserResourceHandle<ResourceAudio> {
					return UserResourceHandle<ResourceAudio>{ hash };
				});
			}
			gui::PayloadSource{ "SOUND_GROUP_HASH", soundGroup.first.get() };
		}
	}

	void SoundGroupWindow::DrawDetails()
	{
		gui::Child soundGroupDetailsChild{ "SoundGroupDetails" };

		gui::TextCenteredUnformatted("Details");
		gui::Separator();

		groupFilename.Draw();
		gui::VarContainer("Sounds", &sounds, [](UserResourceHandle<ResourceAudio>& soundHandle) -> void {
			editor::EditorUtil_DrawResourceHandle("Sound", soundHandle);
		});

		if (gui::Button{ "Create/Modify" })
		{
			std::string filepath{ VFS::JoinPath(Filepaths::soundGroupFolder, groupFilename.GetBuffer() + ".sg")};
			Serializer writer{ VFS::ConvertVirtualToPhysical(filepath) };

			writer.StartArray("sounds");
			for (const auto& handle : sounds)
				writer.Serialize("", handle.GetHash());
			writer.EndArray();

			if (!writer.SaveAndClose())
			{
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::PopupOpenRequest{ "Failed to create sound group", "Could not save file. Asset not imported: " + filepath });
				return;
			}

			ResourceImporter::Import(filepath);
		}
	}
}
