/******************************************************************************/
/*!
\file   Import.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
	Handles the importing of various assets into the game engine through
	dragging and dropping from Windows file explorer.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <shellapi.h> // For DragQueryFile

#include "ResourceManager.h"
#include "AssetBrowser.h"
#include "Engine.h"

// Thanks Microsoft
#ifdef CopyFile
#undef CopyFile
#endif

namespace import
{
	/*****************************************************************//*!
	\enum class IMPORT_RESULT
	\brief
		Indicates the result of an import action.
	*//******************************************************************/
	enum class IMPORT_RESULT
	{
		SUCCESS,
		NO_EXTENSION,
		UNSUPPORTED_EXTENSION,
		ALREADY_IMPORTED
	};

	/*****************************************************************//*!
	 \brief
		   Function will check the amount of files being dragged and dropped 
		   into the window. 
		   It will read the file path of those files and copy them to the
		   designation file location.

	 \return
		   void
	*//******************************************************************/
	void DropCallback(GLFWwindow* window, int count, const char** paths);

	/*****************************************************************//*!
	\brief
		Attempts to import a file as an asset.
	\param file
		The filepath to the file.
	\param resultantPath
		The destination of the file will be written here if successful.
	\return
		The result of the import.
	*//******************************************************************/
	IMPORT_RESULT ImportToAssets(const std::filesystem::path& file, std::filesystem::path* resultantPath = nullptr);

	/*****************************************************************//*!
	\brief
		Copies a file to a destination location.
	\param file
		The filepath to the file.
	\param dest
		The destination.
	*//******************************************************************/
	void CopyFile(const std::filesystem::path& file, const std::filesystem::path& dest);
}
