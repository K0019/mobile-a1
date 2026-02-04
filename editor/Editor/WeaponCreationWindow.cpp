#include "Editor/WeaponCreationWindow.h"
#include "VFS/VFS.h"
#include "Events/EventsQueue.h"
#include "Events/EventsTypeEditor.h"
#include "Assets/AssetImporter.h"
#include "FilepathConstants.h"

namespace editor {

	WeaponCreationWindow::WeaponCreationWindow(const std::string& filename, const WeaponInfo* weaponFile)
		: WindowBase{ "Create Weapon", gui::Vec2{ 400, 200 } }
		, filename{ "Weapon Name", filename }
		, weapon{ weaponFile ? *weaponFile : WeaponInfo{} }
	{
	}

	void WeaponCreationWindow::DrawWindow()
	{
		filename.Draw();
		gui::Separator();

		weapon.EditorDraw();

		if (gui::Button createButton{ "Create/Modify" })
		{
			std::string filepath{ GetFilepath().string() };
			Serializer writer{ VFS::ConvertVirtualToPhysical(filepath) };
			weapon.Serialize(writer);
			if (!writer.SaveAndClose())
			{
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::PopupOpenRequest{ "Failed to create weapon", "Could not save file. Asset not imported: " + filepath });
				return;
			}

			AssetImporter::Import(filepath);
			SetIsOpen(false);
		}
	}

	std::filesystem::path WeaponCreationWindow::GetFilepath() const
	{
		return std::filesystem::path{ Filepaths::gameWeaponSave } / (filename.GetBuffer() + ".weapon");
	}
}
