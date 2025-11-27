/******************************************************************************/
/*!
\file   AndroidInputManager.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This is the header file that contains the declaration of the androidInputManager.
      Currently it only handles a virtual joystick control.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Utilities/Serializer.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"

class AndroidUIDeleterComp
    : public IRegisteredComponent<AndroidUIDeleterComp>
    , public IEditorComponent<AndroidUIDeleterComp>
    , public IGameComponentCallbacks<AndroidUIDeleterComp>
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
property_begin(AndroidUIDeleterComp)
{
}
property_vend_h(AndroidUIDeleterComp)