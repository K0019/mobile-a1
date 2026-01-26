#include "Weapon.h"
#include "Editor/EditorUtilResource.h"
#include "Editor/Containers/GUICollection.h"

void WeaponMoveInfo::EditorDraw()
{
	editor::EditorUtil_DrawResourceHandle("Animation", anim);
	gui::VarDefault("Hitbox Offset", &hitboxOffset);
	gui::VarDefault("Hitbox Extent", &hitboxExtents);
	gui::VarDefault("Hit Delay", &hitDelay);
}

bool WeaponInfo::IsLoaded()
{
	return isLoaded;
}

void WeaponInfo::EditorDraw()
{
	gui::VarContainer("Moves", &moves, [](WeaponMoveInfo& moveInfo) -> bool {
		moveInfo.EditorDraw();
		return false;
	});
	gui::VarDefault("Light DMG", &lightDamage);
	gui::VarDefault("Heavy DMG", &heavyDamage);
	gui::VarDefault("Parry DMG", &parryDamage);
}

void WeaponInfo::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);
	isLoaded = true;
}
