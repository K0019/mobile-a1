#include "EditorUtilResource.h"
#include "Editor/Containers/GUICollection.h"

namespace editor {

	namespace internal {
		template <typename ResourceType>
		void DrawResourceHandle(const char* label, UserResourceHandle<ResourceType>& resourceHandle, const char* payloadIdentifier)
		{
			std::string resourceName{ "None" };
			if (const auto textureResource{ resourceHandle.GetResource() })
				if (const auto registeredResourceName{ ST<MagicResourceManager>::Get()->Editor_GetName(resourceHandle.GetHash()) })
					resourceName = *registeredResourceName;
			gui::TextBoxReadOnly(label, resourceName);
			gui::PayloadTarget<size_t>(payloadIdentifier, [&resourceHandle](size_t hash) -> void {
				resourceHandle = hash;
			});
		}
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceTexture>& resourceHandle)
	{
		internal::DrawResourceHandle(label, resourceHandle, "TEXTURE_HASH");
	}
	template<>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceMaterial>& resourceHandle)
	{
		internal::DrawResourceHandle(label, resourceHandle, "MATERIAL_HASH");
	}
	template<>
	void EditorUtil_DrawResourceHandle(const char* label, UserResourceHandle<ResourceAnimation>& resourceHandle)
	{
		internal::DrawResourceHandle(label, resourceHandle, "ANIMATION_HASH");
	}

}