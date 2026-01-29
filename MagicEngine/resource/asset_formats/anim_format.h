// resource/asset_formats/anim_format.h
#pragma once
#include <cstdint>

namespace Resource {

// Magic number for animation files
constexpr uint32_t ANIM_FILE_MAGIC = 'ANIM';  // 0x4D494E41

#pragma pack(push, 1)

/**
 * @brief Header at the start of .anim files
 *
 * Binary layout:
 * [AnimationFileHeader]
 * [Bone Channels Array]
 * [Skeletal Keyframe Blob]       - Binary blob of all keyframes
 * [Skeletal Name Buffer]
 * [Morph Channels Array]
 * [Morph Keys Array]
 * [Morph Index Buffer]
 * [Morph Weight Buffer]
 */
struct AnimationFileHeader
{
    uint32_t magic = ANIM_FILE_MAGIC;
    float duration;         // In ticks
    float ticksPerSecond;

    // Skeletal animation data
    uint32_t numChannels;
    uint64_t channelDataOffset;
    uint64_t keyframeDataOffset;       // Binary blob
    uint32_t skeletalNameBufferSize;
    uint64_t skeletalNameBufferOffset;

    // Morph animation data
    uint32_t numMorphChannels;
    uint64_t morphChannelDataOffset;
    uint64_t morphKeyDataOffset;
    uint64_t morphIndexDataOffset;
    uint32_t morphWeightDataBufferSize;
    uint64_t morphWeightDataOffset;
};

/**
 * @brief Bone animation channel
 */
struct BoneAnimationChannel
{
    uint32_t nameOffset;  // Into skeletal name buffer

    uint32_t numPositionKeys;
    uint32_t numRotationKeys;
    uint32_t numScaleKeys;

    uint64_t positionKeyOffset;  // Into keyframe blob
    uint64_t rotationKeyOffset;  // Into keyframe blob
    uint64_t scaleKeyOffset;     // Into keyframe blob
};

/**
 * @brief Morph animation channel
 */
struct AnimationFile_MorphChannel
{
    uint32_t meshIndex;  // Which mesh this animates
    uint32_t numKeys;

    uint64_t keyOffset;  // Into morph key data
};

/**
 * @brief Morph animation keyframe
 */
struct AnimationFile_MorphKey
{
    float time;
    uint32_t numTargets;  // Number of morph targets active

    uint64_t targetIndexOffset;  // Into morph index data
    uint64_t weightOffset;        // Into morph weight data
};

#pragma pack(pop)

} // namespace Resource
