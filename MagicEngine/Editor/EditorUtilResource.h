#pragma once
#define EDITOR_UTIL_RESOURCE
#include "Assets/AssetHandle.h"
#include "Assets/Types/AssetTypes.h"
#include "Assets/Types/AssetTypesAudio.h"

namespace editor {

	// ============================================================================
	// Payload identifier traits - maps resource types to drag-drop payload names
	// ============================================================================
	template <typename ResourceType>
	struct AssetPayloadTraits {
		static constexpr const char* PayloadId = nullptr;
	};

	template <> struct AssetPayloadTraits<ResourceTexture>    { static constexpr const char* PayloadId = "TEXTURE_HASH"; };
	template <> struct AssetPayloadTraits<ResourceMaterial>   { static constexpr const char* PayloadId = "MATERIAL_HASH"; };
	template <> struct AssetPayloadTraits<ResourceMesh>       { static constexpr const char* PayloadId = "MESH_HASH"; };
	template <> struct AssetPayloadTraits<ResourceAnimation>  { static constexpr const char* PayloadId = "ANIMATION_HASH"; };
	template <> struct AssetPayloadTraits<ResourceAudio>      { static constexpr const char* PayloadId = "SOUND_HASH"; };
	template <> struct AssetPayloadTraits<ResourceAudioGroup> { static constexpr const char* PayloadId = "SOUND_GROUP_HASH"; };

	// ============================================================================
	// Unified asset slot drawing - works with raw hashes (size_t)
	// Displays "None" for 0, asset name if found, or "<Invalid>" if hash doesn't resolve
	// ============================================================================
	template <typename ResourceType>
	void EditorUtil_DrawAssetSlot(const char* label, size_t& hash);

	// Explicit instantiations declared here, defined in .cpp
	extern template void EditorUtil_DrawAssetSlot<ResourceTexture>(const char* label, size_t& hash);
	extern template void EditorUtil_DrawAssetSlot<ResourceMaterial>(const char* label, size_t& hash);
	extern template void EditorUtil_DrawAssetSlot<ResourceMesh>(const char* label, size_t& hash);
	extern template void EditorUtil_DrawAssetSlot<ResourceAnimation>(const char* label, size_t& hash);
	extern template void EditorUtil_DrawAssetSlot<ResourceAudio>(const char* label, size_t& hash);
	extern template void EditorUtil_DrawAssetSlot<ResourceAudioGroup>(const char* label, size_t& hash);

	// ============================================================================
	// AssetHandle-based drawing (existing API, now uses unified internals)
	// ============================================================================
	template <typename ResourceType>
	void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceType>& resourceHandle)
	{
		static_assert(always_false<ResourceType>::value, "EditorUtil_DrawResourceHandle not implemented for this type");
	}

	template <> void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceTexture>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceMaterial>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAnimation>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAudio>& resourceHandle);
	template <> void EditorUtil_DrawResourceHandle(const char* label, AssetHandle<ResourceAudioGroup>& resourceHandle);

}
