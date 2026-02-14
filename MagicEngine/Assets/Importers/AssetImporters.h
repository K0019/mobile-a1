/******************************************************************************/
/*!
\file   AssetImporters.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
Declarations for all resource import functions. Each function handles importing
a specific file type into the resource system.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <string>

namespace AssetImporters {

    // Import a KTX/KTX2 texture file
    bool ImportKTX(const std::string& assetRelativeFilepath);

    // Import a .material file
    bool ImportMaterial(const std::string& assetRelativeFilepath);

    // Import an audio file (mp3, wav)
    bool ImportAudio(const std::string& assetRelativeFilepath);

    // Import an audio group
    bool ImportAudioGroup(const std::string& assetRelativeFilepath);

    // Import a compiled .mesh file
    bool ImportMeshAsset(const std::string& assetRelativeFilepath);

    // Import a compiled .anim file
    bool ImportAnimationAsset(const std::string& assetRelativeFilepath);

    // Import and compile an FBX/GLB file (delegates to compiler then other importers)
    bool ImportFBX(const std::string& assetRelativeFilepath);

    // Import and compile an image file (png, jpg, etc.) to KTX2
    bool ImportImage(const std::string& assetRelativeFilepath);

    // Import a video file (mp4, webm, mkv, etc.)
    bool ImportVideo(const std::string& assetRelativeFilepath);

} // namespace AssetImporters
