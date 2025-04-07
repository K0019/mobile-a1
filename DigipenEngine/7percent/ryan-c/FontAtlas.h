#pragma once

/******************************************************************************/
/*!
\file   FontAtlas.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of the font atlas structure. Contains glyph information and metrics. JSON file is used to load the font atlas.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& pair) const {
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);
        return hash1 ^ (hash2 << 1); // Combine the two hash values
    }
};

struct Glyph {
    std::array<Vector2, 2> planeBounds;  // Top-left and bottom-right UV coordinates
    std::array<Vector2, 2> atlasBounds;  // Top-left and bottom-right pixel coordinates in the atlas
    float advance;                          // Horizontal advance for the glyph
};

struct FontAtlas {
    static constexpr size_t FIRST_CHAR = 32;  // Space
    static constexpr size_t LAST_CHAR = 126;  // Tilde
    static constexpr size_t CHAR_COUNT = LAST_CHAR - FIRST_CHAR + 1;

    std::array<Glyph, CHAR_COUNT> glyphs;               // Array of glyphs
    std::unordered_map<std::pair<uint32_t, uint32_t>, float, pair_hash> kerningMap;

    // Atlas properties
    std::string textureName;  // Name of the texture file
    uint32_t textureID;       // ID of the texture
    int width;              // Width of the font atlas
    int height;             // Height of the font atlas

    // Metrics
    float ascender;        // Distance from the baseline to the top of the font
    float descender;       // Distance from the baseline to the bottom of the font

    float getKerning(uint32_t char1, uint32_t char2) const;
};


