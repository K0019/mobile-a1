/******************************************************************************/
/*!
\file   AndroidUIDeleter.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
      Deletes the attached entity if we are not on Android. This is good for 
      hiding Android UI, etc.


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "AndroidUIDeleter.h"


void AndroidUIDeleterComp::OnStart()
{
#ifndef __ANDROID__
    ecs::DeleteEntity(ecs::GetEntity(this));
#endif
}


void AndroidUIDeleterComp::EditorDraw()
{
}
