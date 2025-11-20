#pragma once
#include <cstdint>
#include "CompilerMath.h"

namespace compiler
{
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
}
