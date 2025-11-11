/******************************************************************************/
/*!
\file   MagicEngine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the MagicEngine class.
The MagicEngine class is responsible for initializing the game engine, running the game loop, and shutting down the engine.
It includes methods for initializing the engine, running the game loop, and shutting down the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

struct Context;
struct FrameData;

/*****************************************************************//*!
\class MagicEngine
\brief
    The manager of the program.
*//******************************************************************/
class MagicEngine
{
public:
    // Prevent copying
    MagicEngine(const MagicEngine&) = delete;
    MagicEngine& operator=(const MagicEngine&) = delete;

    // Allow moving
    MagicEngine(MagicEngine&&) = default;
    MagicEngine& operator=(MagicEngine&&) = default;

    void Init(Context& context);
    void ExecuteFrame(FrameData& frameData);
    void shutdown();

private:
    void LoadPermanentSystems();

    void ExecuteUpdateSystems();
    void ExecuteRenderSystems();

};
