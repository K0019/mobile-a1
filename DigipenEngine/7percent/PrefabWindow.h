/******************************************************************************/
/*!
\file   PrefabWindow.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
Declaration of Prefab Window class. Currently unused.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ResourceManager.h"
#include "PrefabManager.h"


class PrefabWindow {
    friend ST<PrefabWindow>;
private:
    bool isLoading; /**< A boolean indicating whether the prompt is open for loading or saving. */
    bool isOpen{ false }; /**< A boolean indicating whether the window is open or not. */
    char prefabName[32]{"New Prefab"};
public:
    /**
     * @brief Default constructor for the SavePrefabPrompt class.
     */
    PrefabWindow();

    /**
     * @brief Draws the prompt.
     *
     * @param p_open A pointer to a boolean indicating whether the asset browser window is open or not.
     */
    void DrawSaveLoadPrompt(bool* p_open);

    void Open(bool loading);
    const char* PrefabName();
    const bool& IsOpen() const;
    bool& IsOpen();
};

