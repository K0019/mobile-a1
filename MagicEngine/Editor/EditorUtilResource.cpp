#include "EditorUtilResource.h"
#include "Editor/Containers/GUICollection.h"
#include "Assets/AssetManager.h"

namespace editor {

	namespace internal {
		std::string GetAssetNameFromHash(size_t hash)
		{
			if (hash == 0)
				return "None";
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(hash))
				return *name;
			return "<Invalid>";
		}
	}

	// ============================================================================
	// Unified asset slot implementation
	// ============================================================================
	template <typename ResourceType>
	void EditorUtil_DrawAssetSlot(const char* label, size_t& hash)
	{
		static_assert(AssetPayloadTraits<ResourceType>::PayloadId != nullptr,
			"AssetPayloadTraits not defined for this resource type");

		gui::TextBoxReadOnly(label, internal::GetAssetNameFromHash(hash));
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceType>::PayloadId, [&hash](size_t newHash) {
			hash = newHash;
		});
	}

	// Explicit instantiations
	template void EditorUtil_DrawAssetSlot<ResourceTexture>(const char* label, size_t& hash);
	template void EditorUtil_DrawAssetSlot<ResourceMaterial>(const char* label, size_t& hash);
	template void EditorUtil_DrawAssetSlot<ResourceMesh>(const char* label, size_t& hash);
	template void EditorUtil_DrawAssetSlot<ResourceAnimation>(const char* label, size_t& hash);
	template void EditorUtil_DrawAssetSlot<ResourceAudio>(const char* label, size_t& hash);
	template void EditorUtil_DrawAssetSlot<ResourceAudioGroup>(const char* label, size_t& hash);

	// ============================================================================
	// AssetHandle-based drawing (uses unified internals)
	// ============================================================================
	template<>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceTexture>& resourceHandle)
	{
		std::string resourceName{ "None" };
		if (resourceHandle.GetResource())
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(resourceHandle.GetHash()))
				resourceName = *name;
		gui::TextBoxReadOnly(label, resourceName);
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceTexture>::PayloadId, [&resourceHandle](size_t hash) {
			resourceHandle = hash;
		});
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceMaterial>& resourceHandle)
	{
		std::string resourceName{ "None" };
		if (resourceHandle.GetResource())
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(resourceHandle.GetHash()))
				resourceName = *name;
		gui::TextBoxReadOnly(label, resourceName);
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceMaterial>::PayloadId, [&resourceHandle](size_t hash) {
			resourceHandle = hash;
		});
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAnimation>& resourceHandle)
	{
		std::string resourceName{ "None" };
		if (resourceHandle.GetResource())
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(resourceHandle.GetHash()))
				resourceName = *name;
		gui::TextBoxReadOnly(label, resourceName);
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceAnimation>::PayloadId, [&resourceHandle](size_t hash) {
			resourceHandle = hash;
		});
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAudio>& resourceHandle)
	{
		std::string resourceName{ "None" };
		if (resourceHandle.GetResource())
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(resourceHandle.GetHash()))
				resourceName = *name;
		gui::TextBoxReadOnly(label, resourceName);
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceAudio>::PayloadId, [&resourceHandle](size_t hash) {
			resourceHandle = hash;
		});
	}

	template<>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAudioGroup>& resourceHandle)
	{
		std::string resourceName{ "None" };
		if (resourceHandle.GetResource())
			if (const auto* name = ST<AssetManager>::Get()->Editor_GetName(resourceHandle.GetHash()))
				resourceName = *name;
		gui::TextBoxReadOnly(label, resourceName);
		gui::PayloadTarget<size_t>(AssetPayloadTraits<ResourceAudioGroup>::PayloadId, [&resourceHandle](size_t hash) {
			resourceHandle = hash;
		});
	}

}