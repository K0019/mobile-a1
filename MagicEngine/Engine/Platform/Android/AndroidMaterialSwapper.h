/******************************************************************************/
/*!
\file   AndroidMaterialSwapper.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
      This is the header file that contains the declaration of the AndroidMaterialSwapper.
      Its sole purpose is to swap materials if on android.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Utilities/Serializer.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"

class AndroidMaterialSwapperComp
    : public IRegisteredComponent<AndroidMaterialSwapperComp>
    , public IEditorComponent<AndroidMaterialSwapperComp>
    , public IGameComponentCallbacks<AndroidMaterialSwapperComp>
{
public:
    /*****************************************************************//*!
    \brief
        Called when the component starts
    *//******************************************************************/
    void OnStart() override;


    virtual void EditorDraw() override; // not in used

    property_vtable()
};
property_begin(AndroidMaterialSwapperComp)
{
}
property_vend_h(AndroidMaterialSwapperComp)