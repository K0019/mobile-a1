/******************************************************************************/
/*!
\file   Import.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Chan Kuan Fu Ryan (50%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Chua Wen Shing Bryan (50%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
    Handles the importing of various assets into the game engine through
    dragging and dropping from Windows file explorer.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Import.h"
#include "AudioManager.h"
#include "Popup.h"

namespace import
{
    void DropCallback(GLFWwindow*, int count, const char** paths)
    {
        std::ostringstream ss           {}; // To use with Popup

        // Loop through all dropped files
        std::filesystem::path src       {}; // Source where the file is copied from
        for (int i = 0; i < count; ++i)
        {
            src = paths[i];
            switch (ImportToAssets(src))
            {
            case IMPORT_RESULT::NO_EXTENSION:
                ss << "No extension found, import skipped: " << src.filename() << std::endl;
                continue;
            case IMPORT_RESULT::UNSUPPORTED_EXTENSION:
                ss << "File type not supported: " << src.filename() << std::endl;
                continue;
            case IMPORT_RESULT::ALREADY_IMPORTED:
                ss << "Source and destination are same, import skipped: " << src.filename() << std::endl;
                continue;
            case IMPORT_RESULT::SUCCESS:
                ss << "Imported successfully: " << src.filename() << std::endl;
                continue;
            }
        }
        ST<Popup>::Get()->OpenWithContent("Import", ss);
    }

    IMPORT_RESULT ImportToAssets(const std::filesystem::path& file, std::filesystem::path* resultantPath)
    {
        // We require an extension to identify the file type.
        if (!file.has_extension())
            return IMPORT_RESULT::NO_EXTENSION;

        Filepaths& filepaths{ *ST<Filepaths>::Get() };
        std::filesystem::path dest{ filepaths.assets };
        std::string extension{ file.extension().string() };
        bool isGrouped{};

        // Ensure extension is in lower case
        std::transform(extension.begin(), extension.end(), extension.begin(), [](char c) -> char { return static_cast<char>(std::tolower(c)); });

        // TODO: Make this better
        // Determine destination
        if (extension == ".png" || extension == ".jpg")
            dest /= "Images";
        else if (extension == ".wav")
        {
            AudioManager& audioManager{ *ST<AudioManager>::Get() };
            isGrouped = audioManager.CheckIsGrouped(file.filename().string());
            dest = (isGrouped ? filepaths.soundSingleFolder : filepaths.soundGroupedFolder);
        }
        else
            return IMPORT_RESULT::UNSUPPORTED_EXTENSION;
        dest /= file.filename();

        // Skip if same destination
        if (file == dest)
            return IMPORT_RESULT::ALREADY_IMPORTED;

        // Copy the file
        CopyFile(file, dest);
        if (resultantPath)
            *resultantPath = dest;

        // Update AudioManager if applicable (this must be done after the copy operation)
        if (extension == ".wav")
            ST<AudioManager>::Get()->CreateUpdateSound(file.filename().string(), isGrouped);

        return IMPORT_RESULT::SUCCESS;
    }

    void CopyFile(const std::filesystem::path& file, const std::filesystem::path& dest)
    {
        std::filesystem::copy_file(
            file,
            std::filesystem::is_directory(dest) ? dest / file.filename() : dest,
            std::filesystem::copy_options::overwrite_existing
        );
    }
}
