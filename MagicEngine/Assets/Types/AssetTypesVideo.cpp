/******************************************************************************/
/*!
\file   AssetTypesVideo.cpp
\brief  Implementation of video resource type.
*/
/******************************************************************************/

#include "Assets/Types/AssetTypesVideo.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetFilepaths.h"
#include "VFS/VFS.h"

bool ResourceVideo::IsLoaded()
{
    // Video is considered loaded if we have valid metadata
    return data.width > 0 && data.height > 0;
}

bool ResourceVideo::EnsureMetadataLoaded()
{
    // Fast path: already loaded
    if (IsLoaded())
        return true;

#ifdef MAGIC_HAS_VIDEO
    // Look up file path from asset manager using inherited hash member
    const AssetFilepaths::FileEntry* entry = ST<AssetManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(hash);
    if (!entry)
        return false;

    // Convert to physical path and read video header
    std::string fullPath = VFS::ConvertVirtualToPhysical(entry->path);

    video::VideoDecoder decoder;
    if (!decoder.open(fullPath))
        return false;

    // Populate metadata from video header
    const video::VideoInfo& info = decoder.getInfo();
    data.name = VFS::GetStem(entry->path);
    data.width = info.width;
    data.height = info.height;
    data.duration = static_cast<float>(info.duration);
    data.frameRate = static_cast<float>(info.frameRate);
    data.hasAudio = info.hasAudio;
    data.codec = info.codec;

    return true;
#else
    return false;
#endif
}

ResourceVideo::~ResourceVideo()
{
    // Video data is managed by VideoManager, nothing to free here
    // The actual video file is loaded on-demand when played
}
