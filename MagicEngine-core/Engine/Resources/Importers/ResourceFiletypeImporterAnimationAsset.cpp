#include "Engine/Resources/Importers/ResourceFiletypeImporterAnimationAsset.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/ResourceManager.h"
#include "GameSettings.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "tools/assets/io/import_config.h"


#pragma region Animation File Structure
// Just copy pasting definitions from assetcompiler/MeshFileStructure.h 
// To make engine not depend on assetcompiler project

constexpr uint32_t ANIM_FILE_MAGIC = { 'ANIM' };

#pragma pack(push, 1)
// Represents a morph keyframe
struct AnimationFile_MorphKey
{
    float    time;
    uint32_t numTargets; // indice/weight count

    uint64_t targetIndexOffset; // Offset from the start of morphIndexDataOffset
    uint64_t weightOffset; // Offset from the start of morphWeightDataOffset
};

// One morph animation
struct AnimationFile_MorphChannel
{
    uint32_t meshIndex;
    uint32_t numKeys;

    uint64_t keyOffset; // Offset from the start of morphKeyDataOffset
};

struct AnimationFileHeader
{
    uint32_t magic = ANIM_FILE_MAGIC;
    float    duration;
    float    ticksPerSecond;

    // Skeletal Data
    uint32_t numChannels;
    uint64_t channelDataOffset;
    uint64_t keyframeDataOffset;
    uint32_t skeletalNameBufferSize;
    uint64_t skeletalNameBufferOffset;

    // Morph Data
    uint32_t numMorphChannels;
    uint64_t morphChannelDataOffset;
    uint64_t morphKeyDataOffset;
    uint64_t morphIndexDataOffset;
    uint32_t morphWeightDataBufferSize;
    uint64_t morphWeightDataOffset;
};

// One skeletal animation
struct BoneAnimationChannel
{
    uint32_t nameOffset;

    uint32_t numPositionKeys;
    uint32_t numRotationKeys;
    uint32_t numScaleKeys;

    uint64_t positionKeyOffset;
    uint64_t rotationKeyOffset;
    uint64_t scaleKeyOffset;
};
#pragma pack(pop)

#pragma endregion

namespace
{
    std::string to_lower(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

namespace internal
{
    bool ImportAnimationAsset(const std::string& filepath, Resource::ProcessedAnimationClip& outClip)
    {
        auto file = VFS::OpenFile(filepath, FileMode::Read);
        if (!file)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "VFS: Failed to open animation file: " << filepath;
            return false;
        }

        //Validate Header
        AnimationFileHeader header;
        file->Read(&header, sizeof(AnimationFileHeader));

        if (header.magic != ANIM_FILE_MAGIC)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid anim asset file format: " << filepath;
            return false;
        }

        // Read file data
        // --- Skeletal Blobs ---
        std::vector<BoneAnimationChannel> boneChannels(header.numChannels);
        file->Seek(header.channelDataOffset, SeekOrigin::Begin);
        file->Read(boneChannels.data(), header.numChannels * sizeof(BoneAnimationChannel));

        size_t skeletalKeyframeBlobSize = header.skeletalNameBufferOffset - header.keyframeDataOffset;
        std::vector<char> skeletalKeyframeBuffer(skeletalKeyframeBlobSize);
        file->Seek(header.keyframeDataOffset, SeekOrigin::Begin);
        file->Read(skeletalKeyframeBuffer.data(), skeletalKeyframeBlobSize);

        std::vector<char> skeletalNameBuffer(header.skeletalNameBufferSize);
        file->Seek(header.skeletalNameBufferOffset, SeekOrigin::Begin);
        file->Read(skeletalNameBuffer.data(), header.skeletalNameBufferSize);

        // --- Morph Blobs ---
        std::vector<AnimationFile_MorphChannel> morphChannels(header.numMorphChannels);
        file->Seek(header.morphChannelDataOffset, SeekOrigin::Begin);
        file->Read(morphChannels.data(), header.numMorphChannels * sizeof(AnimationFile_MorphChannel));

        size_t morphKeyBlobSize = header.morphIndexDataOffset - header.morphKeyDataOffset;
        std::vector<char> morphKeyBuffer(morphKeyBlobSize);
        file->Seek(header.morphKeyDataOffset, SeekOrigin::Begin);
        file->Read(morphKeyBuffer.data(), morphKeyBlobSize);

        size_t morphIndexBlobSize = header.morphWeightDataOffset - header.morphIndexDataOffset;
        std::vector<char> morphIndexBuffer(morphIndexBlobSize);
        file->Seek(header.morphIndexDataOffset, SeekOrigin::Begin);
        file->Read(morphIndexBuffer.data(), morphIndexBlobSize);

        // For the last blob, we read from its offset to the end of the file
        size_t morphWeightBlobSize = header.morphWeightDataBufferSize;
        std::vector<char> morphWeightBuffer(morphWeightBlobSize);
        file->Seek(header.morphWeightDataOffset, SeekOrigin::Begin);
        file->Read(morphWeightBuffer.data(), morphWeightBlobSize);

        // Create the animation clip
        outClip.name = std::filesystem::path(filepath).stem().string();
        outClip.duration = header.duration;
        outClip.ticksPerSecond = header.ticksPerSecond;

        // --- Un-flatten Skeletal Channels ---
        outClip.skeletalChannels.resize(header.numChannels);
        for (uint32_t i = 0; i < header.numChannels; ++i)
        {
            const auto& fileChannel = boneChannels[i];
            auto& engineChannel = outClip.skeletalChannels[i]; // This is ProcessedAnimationChannel

            // Find the name in the name buffer
            engineChannel.nodeName = &skeletalNameBuffer[fileChannel.nameOffset];

            // Find the keyframes in the keyframe buffer
            const Resource::ProcessedAnimationVectorKey* posKeys = reinterpret_cast<const Resource::ProcessedAnimationVectorKey*>(&skeletalKeyframeBuffer[fileChannel.positionKeyOffset]);
            const Resource::ProcessedAnimationQuatKey* rotKeys = reinterpret_cast<const Resource::ProcessedAnimationQuatKey*>(&skeletalKeyframeBuffer[fileChannel.rotationKeyOffset]);
            const Resource::ProcessedAnimationVectorKey* scaleKeys = reinterpret_cast<const Resource::ProcessedAnimationVectorKey*>(&skeletalKeyframeBuffer[fileChannel.scaleKeyOffset]);

            // Copy the keyframe data
            engineChannel.translationKeys.assign(posKeys, posKeys + fileChannel.numPositionKeys);
            engineChannel.rotationKeys.assign(rotKeys, rotKeys + fileChannel.numRotationKeys);
            engineChannel.scaleKeys.assign(scaleKeys, scaleKeys + fileChannel.numScaleKeys);
        }

        // --- Un-flatten Morph Channels ---
        outClip.morphChannels.resize(header.numMorphChannels);
        for (uint32_t i = 0; i < header.numMorphChannels; ++i)
        {
            const auto& fileChannel = morphChannels[i];
            auto& engineChannel = outClip.morphChannels[i]; // This is ProcessedMorphChannel

            engineChannel.meshIndex = fileChannel.meshIndex;
            engineChannel.keys.resize(fileChannel.numKeys);

            // Find this channel's keys in the key blob
            const AnimationFile_MorphKey* fileKeys = reinterpret_cast<const AnimationFile_MorphKey*>(&morphKeyBuffer[fileChannel.keyOffset]);

            // Loop through each key to un-flatten its indices and weights
            for (uint32_t j = 0; j < fileChannel.numKeys; ++j)
            {
                const auto& fileKey = fileKeys[j];
                auto& engineKey = engineChannel.keys[j]; // This is ProcessedMorphKey

                engineKey.time = fileKey.time;

                // Find this key's indices and weights in their respective blobs
                const uint32_t* indices = reinterpret_cast<const uint32_t*>(&morphIndexBuffer[fileKey.targetIndexOffset]);
                const float* weights = reinterpret_cast<const float*>(&morphWeightBuffer[fileKey.weightOffset]);

                // Copy the data
                engineKey.targetIndices.assign(indices, indices + fileKey.numTargets);
                engineKey.weights.assign(weights, weights + fileKey.numTargets);
            }
        }

        return true;
    }

    void SetResourceHandlesAnimation(const std::vector<AssociatedResourceHashes>& resourceHashes, Resource::ClipId clipId)
    {
        auto& anims{ ST<MagicResourceManager>::Get()->INTERNAL_GetContainer< ResourceAnimation>() };
        const auto& animHashes { resourceHashes[0].hashes };

        ResourceAnimation* anim = { anims.INTERNAL_GetResource(animHashes[0], true) };
        anim->handle = clipId;
    }
}


bool ResourceFiletypeImporterAnimationAsset::Import(const std::string& assetRelativeFilepath)
{
    Resource::ProcessedAnimationClip clipToLoad;

    if (!internal::ImportAnimationAsset(assetRelativeFilepath, clipToLoad))
    {
        return false;
    }

    // Upload to GPU
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    Resource::ClipId clipId = graphicsAssetSystem.createClip(clipToLoad);

    if (clipId == Resource::INVALID_CLIP_ID)
    {
        return false;
    }

    const auto* fileEntry{ GenerateFileEntryForResources<ResourceAnimation>(assetRelativeFilepath, 1) };
    internal::SetResourceHandlesAnimation(fileEntry->associatedResources, clipId);

    return true;
}
