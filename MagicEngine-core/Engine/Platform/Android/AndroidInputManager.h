#pragma once
#include "Utilities/Serializer.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "core/platform/platform.h"

class AndroidInputComp
    : public IRegisteredComponent<AndroidInputComp>
    , public IEditorComponent<AndroidInputComp>
    , public IGameComponentCallbacks<AndroidInputComp>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    // tunables
    float baseRadius = 160.0f;     // px
    float deadZoneFrac = 0.12f;      // of baseRadius
    bool  dynamicCenter = true;

    // output in [-1,1]
    const Vec2& GetValue() const { return m_value; }
    bool IsActive() const { return m_active; }

    AndroidInputComp();



public:
    /*****************************************************************//*!
    \brief
        Called when the component is attached to an entity.
    *//******************************************************************/
    void OnAttached() override;

    void OnStart() override;

    /*****************************************************************//*!
    \brief
        Called when the component is detached from an entity.
    *//******************************************************************/
    void OnDetached() override;
    /*****************************************************************//*!
    \brief
         Updates the input with each tick
    *//******************************************************************/
    void Update();

private:

    virtual void EditorDraw() override;
    Vec2 m_center{ 0.f,0.f };
    Vec2 m_value{ 0.f,0.f };
    bool m_active = false;

    void placeInitialCenter();

};