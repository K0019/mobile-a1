#pragma once
/******************************************************************************/
/*!
\file   Sprite.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Sprite data structure that holds information about the sprite. Used for serialisation and deserialisation in the ResourceManager.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

class Sprite
{
public:
  std::string name;
  std::string textureName;

  uint32_t textureID;
  // DO NOT MISTAKE THIS FOR THE ID OF THE TEXTURE IN THE RESOURCE MANAGER.
  // THIS ID IS USED SOLELY FOR RENDERING AND CANNOT BE USED TO RETRIEVE ANY INFORMATION FROM ANY OF THE SYSTEMS IN THE GAME

  uint32_t width;
  uint32_t height;
  Vector4 texCoords{ 0.0f, 0.0f, 1.0f, 1.0f };
};

