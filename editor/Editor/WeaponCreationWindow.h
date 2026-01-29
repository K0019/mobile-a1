#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Game/Weapon.h"

namespace editor {

	class WeaponCreationWindow : public WindowBase<WeaponCreationWindow, false>
	{
	public:
		WeaponCreationWindow(const std::string& filename, const WeaponInfo* weaponFile);

		void DrawWindow() override;

	private:
		std::filesystem::path GetFilepath() const;

	private:
		gui::TextBoxWithBuffer<64> filename;
		WeaponInfo weapon;

	};

}
