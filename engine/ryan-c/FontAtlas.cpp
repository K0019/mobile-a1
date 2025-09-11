/******************************************************************************/
/*!
\file   FontAtlas.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Has the definition of the getKerning function.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "FontAtlas.h"

float FontAtlas::getKerning(uint32_t char1, uint32_t char2) const {
    auto it = kerningMap.find({char1, char2});
    if (it != kerningMap.end()) {
        return it->second;  // Return the found kerning value
    }
    return 0.0f;  // Default kerning if not found
}
