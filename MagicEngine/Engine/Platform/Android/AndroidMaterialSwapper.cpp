/******************************************************************************/
/*!
\file   AndroidMaterialSwapper.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
      Swaps material of the attached entity if we are on Android. This is good for
      tutorial prompts, etc.


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "AndroidMaterialSwapper.h"
#include "MaterialSwapper.h"


void AndroidMaterialSwapperComp::OnStart()
{
#ifdef __ANDROID__
    if (ecs::CompHandle<MaterialSwapperComponent> matComp{ ecs::GetEntity(this)->GetComp<MaterialSwapperComponent>() })
    {
        matComp->ToggleMaterialSwap(true);
    }
#endif
}


void AndroidMaterialSwapperComp::EditorDraw()
{
}
