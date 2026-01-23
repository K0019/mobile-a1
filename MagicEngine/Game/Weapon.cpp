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

void WeaponInfo::EditorDraw()
{
	gui::VarContainer("Moves", &moves, [](WeaponMoveInfo& moveInfo) -> void { moveInfo.EditorDraw(); });
	gui::VarDefault("Light DMG", &lightDamage);
	gui::VarDefault("Heavy DMG", &heavyDamage);
	gui::VarDefault("Parry DMG", &parryDamage);
}