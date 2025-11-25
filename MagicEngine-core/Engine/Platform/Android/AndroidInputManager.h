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
#include "core/platform/platform.h"

#include "Engine/Platform/Android/AndroidInputBridge.h"

/*****************************************************************//*!
// AndroidInputComp
//   - A light “virtual joystick” component intended for Android
//   - Consumes AndroidInputBridge::State() each frame
//   - Produces a normalized 2D vector in [-1,1]x[-1,1] (m_value)
//   - Optional dynamic centering and activation zone
*//******************************************************************/
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
    float baseRadius = 160.0f;                  // logical joystick radius (pre-scale)
    float deadZoneFrac = 0.12f;                 // [0..1) size of dead zone as a fraction of radius
    bool  dynamicCenter = true;                 // if true, first press can re-center joystick at touch

    // Touch must begin inside this rect to activate/capture the joystick (when dynamicCenter=true).
    Vec2  dynZoneMin{ 120.f, 150.f };           
    Vec2  dynZoneMax{ 350.f, 300.f };           
    float radiusScale = 1.0f;        // global scale multiplier applied to baseRadius
    float captureRadiusMul = 1.0f;   // capture circle size, in multiples of EffectiveRadius()
    float dirDead = 0.30f;           // normalized threshold for mapping to W/A/S/D labels

    float followWorld = 100.25f;   // how far to nudge at full deflection 
    bool  recenterOnRelease = true; // snap back to anchor on release

    // ============================ Rotation / coordinate fixups (only used by PhoneToScreen()) ========================
    enum class TouchRot { Rot0, Rot90CW, Rot90CCW };
    TouchRot touchRotation = TouchRot::Rot90CW; // default to 90 CW to match current android build
    //bool    flipY = false;                       //  rarely needed for Android surface coordinates
    //==================================================================================================================
    
    // ============================To capture Joystick on runtime ======================
    bool  m_captured = false;        // true if press began within “capture” region
    std::string m_prevDir;           // last printed label ("W","WA",...)
    Vec3  m_anchorWorld{ 0.f, 0.f, 0.f };  // cached at press
    //==================================================================================

    //Joystick vector in normalized units; length in [0..1]
    /*****************************************************************//*!
    \brief
        Return current val
    *//******************************************************************/
    const Vec2& GetValue() const { return m_value; }
    /*****************************************************************//*!
    \brief
        check if the stick is currently engaged 
    *//******************************************************************/
    bool IsActive() const { return m_active; }

    AndroidInputComp();
public:
    /*****************************************************************//*!
    \brief
        Called when the component is attached to an entity.
    *//******************************************************************/
    void OnAttached() override;

    /*****************************************************************//*!
    \brief
        Called when the component starts
    *//******************************************************************/
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

    virtual void EditorDraw() override; // not in used

    // =============================== Internal state ===============================

    Vec2 m_center{ 0.f,0.f };           // joystick center in *screen* coords
    Vec2 m_value{ 0.f,0.f };            // output vector in normalized units
    bool m_active = false;              // true while the finger is held and this stick is engaged
    //==================================================================================

    // =============================== Helpers ===============================

    /*****************************************************************//*!
    \brief
          Choose a sensible default joystick center so the entire stick stays on
          screen. Uses EffectiveRadius() and a small margin from the edges
    *//******************************************************************/
    void placeInitialCenter();

    /*****************************************************************//*!
    \brief
         Compute the active joystick radius after applying the global scale
    *//******************************************************************/
    float EffectiveRadius() const;

    /*****************************************************************//*!
    \brief
        Convert the fractional dead zone to pixels using the effective radius
    \return
        Dead-zone size in pixels.
    *//******************************************************************/
    float DeadZonePixels() const;

    /*****************************************************************//*!
    \brief
    Rotate/flip raw phone coordinates into the engine’s screen space so that
    joystick math and labels (W/A/S/D) align with your render orientation
    \param p
        Raw phone-space position (origin top-left, +y downward)
    \return
        Screen-space position after rotation/flip
    *//******************************************************************/
    Vec2 PhoneToScreen(Vec2 p) const;
    //==================================================================================

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
