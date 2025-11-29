#pragma once
#define EDITOR_UTIL_RESOURCE
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Utilities/Logging.h"

namespace editor {

	template <typename ResourceType>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceType>& resourceHandle)
	{
		static_assert(false, "EditorUtil_DrawResourceHandle not implemented for this type");
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceTexture>& resourceHandle)
	{
		std::string textureName{ "None" };
		if (const auto textureResource{ resourceHandle.GetResource() })
			if (const auto resourceName{ ST<MagicResourceManager>::Get()->Editor_GetName(resourceHandle.GetHash()) })
				textureName = *resourceName;
		gui::TextBoxReadOnly(label, textureName);
		gui::PayloadTarget<size_t>("TEXTURE_HASH", [&resourceHandle](size_t hash) -> void {
			resourceHandle = hash;
		});
	}

}
