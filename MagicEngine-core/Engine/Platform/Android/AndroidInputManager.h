#pragma once
#include "Utilities/Serializer.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "core/platform/platform.h"

#include "Engine/Platform/Android/AndroidInputBridge.h"

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
    // Tunables
    float baseRadius = 160.0f;     // px
    float deadZoneFrac = 0.12f;   // fraction of baseRadius
    bool  dynamicCenter = true;
    Vec2  dynZoneMin{ 120.f, 150.f };           // activation zone (screen coords)
    Vec2  dynZoneMax{ 350.f, 300.f };           // activation zone (screen coords)
    float radiusScale = 1.0f;
    float captureRadiusMul = 1.0f;   // how big the capture circle is, in multiples of EffectiveRadius()
    float dirDead = 0.30f;           // normalized threshold to consider W/A/S/D “on

    //============================FOR CHECKING ROTATION
    enum class TouchRot { Rot0, Rot90CW, Rot90CCW };
    TouchRot touchRotation = TouchRot::Rot90CW; // <-- matches your symptom
    bool    flipY = false;                       // usually false for Android window coords
    //============================
    
     //============================ TO CAPTURE JOYSTICK THINGS
    // Runtime
    bool  m_captured = false;        // pressed inside joystick region?
    std::string m_prevDir;           // last printed label ("W","WA",...)
    //============================
    

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
    float EffectiveRadius() const;
    float DeadZonePixels() const;
    Vec2 PhoneToScreen(Vec2 p) const;

};

/*****************************************************************//*!
\brief
      ECS system responsible for updating all AndroidInputComp
*//******************************************************************/
class AndroidInputSystem
    : public ecs::System<AndroidInputSystem, AndroidInputComp>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    AndroidInputSystem();

private:
    /*****************************************************************//*!
    \brief
        Updates a single BehaviorTreeComp instance.
    \param
        The component to update
    *//******************************************************************/
    void UpdateComp(AndroidInputComp& comp);
};
//=======================================================================
