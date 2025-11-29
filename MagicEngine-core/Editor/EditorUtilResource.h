#pragma once
#define EDITOR_UTIL_RESOURCE
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

namespace editor {

	// Specializations implemented in CPP. As long as the ResourceType is listed below, this will work.
	template <typename ResourceType>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceType>& resourceHandle)
	{
		static_assert(false, "EditorUtil_DrawResourceHandle not implemented for this type");
	}

	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceTexture>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceMaterial>& resourceHandle);

}
