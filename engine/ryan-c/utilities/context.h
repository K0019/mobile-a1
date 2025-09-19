/******************************************************************************/
/*!
\file   context.h
\par    Project: MagicEngine
\par    Course: CSD3401
\par    Section B
\par    Software Engineering Project 5
\date   09/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A "Context" class is used often within the graphics interface by Ryan Cheong.
This was originally in a terrible place, so I've moved it over here for better organization.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

class Renderer; // renderer.h
namespace AssetLoading {
	class AssetSystem; // asset_system.h
}

/*****************************************************************//*!
\struct Context
\brief
	A simple container for things that the graphics interface requires.
*//******************************************************************/
struct Context
{
	Renderer* renderer{};
	AssetLoading::AssetSystem* assetSystem{};
};

