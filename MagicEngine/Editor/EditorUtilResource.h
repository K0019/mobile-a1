#pragma once
#define EDITOR_UTIL_RESOURCE
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Game/Weapon.h"

namespace editor {

	// Specializations implemented in CPP. As long as the ResourceType is listed below, this will work.
	template <typename ResourceType>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceType>& resourceHandle)
	{
		static_assert(always_false<ResourceType>::value, "EditorUtil_DrawResourceHandle not implemented for this type");
	  // note, always_false is defined in properties.h, it's not an actual cpp type
	  // the bug comes from different versions of the compiler evaluating static_assert at different times
	  // previously it would be evaluated at compile time, even without a function instantiation. Putting a template forces it to only be evaluated when instantiated.
	}

	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceTexture>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceMaterial>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceAnimation>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<WeaponInfo>& resourceHandle);

}
