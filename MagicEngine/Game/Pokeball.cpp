#include "Pokeball.h"
#include "Engine/Platform/Android/AndroidInputBridge.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Input.h"
#include "Engine/PrefabManager.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"
#include "core/platform/platform.h"

#include "Physics/Physics.h"
#include "Game/Target.h"
#include "Game/GyroCamera.h"

PokeballComponent::PokeballComponent()
    : isThrown{}
{
}

bool PokeballComponent::GetIsThrown() const
{
    return isThrown;
}
void PokeballComponent::SetThrown()
{
    isThrown = true;
    launchTime = GameTime::TimeSinceStart();
}

float PokeballComponent::GetTimeInAir() const
{
    return GameTime::TimeSinceStart() - launchTime;
}

void PokeballComponent::OnTargetHit(ecs::EntityHandle targetEntity)
{
    CONSOLE_LOG(LEVEL_INFO) << "TARGET HIT!";
    ecs::DeleteEntity(targetEntity);
    ST<EventsQueue>::Get()->AddEventForNextFrame(Events::Game_NiceThrow{});

    // Let respawn system handle our ball respawning
    launchTime = -999.0f;
    ecs::GetEntityTransform(this).SetWorldPosition(Vec3{ 0.0f, -999.0f, 0.0f });
}

PokeballThrowSystem::PokeballThrowSystem()
	: System_Internal{ &PokeballThrowSystem::UpdateComp }
    , pressed{}, released{}
{
}

bool PokeballThrowSystem::PreRun()
{
#ifdef __ANDROID__
    // Look for any unowned pointer that just went down
    int downPid = AndroidInputBridge::FindUnownedJustDown();
    pressed = (downPid >= 0);

    // Check if any pointer just went up (for release detection)
    released = false;
    for (int i = 0; i < AndroidInputBridge::MAX_POINTERS; ++i) {
        if (AndroidInputBridge::State(i).justUp) {
            released = true;
            break;
        }
    }

    if (pressed) {
        const auto& ts = AndroidInputBridge::State(downPid);
        m_activePid = downPid;
        // Convert to UI space
        Vec2 rawPos{ ts.x, ts.y };
        float uiX, uiY;
        if (!ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY))
            return false;
        downPos = pos = Vec2{ uiX, uiY };
        return true;
    }
    else if (released) {
        // Find a pointer that just went up and get its position
        for (int i = 0; i < AndroidInputBridge::MAX_POINTERS; ++i) {
            if (AndroidInputBridge::State(i).justUp) {
                const auto& ts = AndroidInputBridge::State(i);
                Vec2 rawPos{ ts.x, ts.y };
                float uiX, uiY;
                if (ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY)) {
                    pos = Vec2{ uiX, uiY };
                    return true;
                }
            }
        }
        return false;
    }
    return false;
#else
    pressed = ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::M_LEFT);
    released = ST<KeyboardMouseInput>::Get()->GetIsReleased(KEY::M_LEFT);

    static int frameCounter = 0;
    static bool lastPressed = false;
    if (pressed != lastPressed || (frameCounter++ % 300 == 0)) {
        Vec2 rawPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
        printf("[ButtonInput] pressed=%d released=%d rawMousePos=(%.1f, %.1f) displaySize=(%dx%d)\n",
            pressed, released, rawPos.x, rawPos.y, Core::Display().GetWidth(), Core::Display().GetHeight());
        fflush(stdout);
        lastPressed = pressed;
    }

    if (!(pressed || released))
        return false;

    Vec2 rawPos = MagicInput::GetMousePos();
    float uiX, uiY;
    if (!ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY))
        return false;
    pos = Vec2{ uiX, uiY };
    if (pressed)
        downPos = pos;
    return true;
#endif
}

void PokeballThrowSystem::UpdateComp(PokeballComponent& comp)
{
    if (!released)
        return;

    Vec2 swipeDir{ pos - downPos };
    if (swipeDir.LengthSqr() <= std::numeric_limits<float>::epsilon())
        return;
    swipeDir = -swipeDir * 0.01f; // Window dir is flipped

    Vec3 launchVec{ swipeDir, 2.0f };
    auto gyroCamIter{ ecs::GetCompsActiveBegin<GyroCameraComponent>() };
    if (gyroCamIter != ecs::GetCompsEnd<GyroCameraComponent>())
        launchVec = glm::rotateY(launchVec, math::ToRadians(gyroCamIter.GetEntity()->GetTransform().GetWorldRotation().y));
    CONSOLE_LOG(LEVEL_INFO) << "Launched pokeball at " << launchVec;

    if (auto physComp{ ecs::GetEntity(&comp)->GetComp<physics::PhysicsComp>() })
    {
        physComp->SetEnabled(true);
        physComp->SetLinearVelocity(launchVec);
        physComp->SetUseGravity(true);
    }
    comp.SetThrown();
}

PokeballRespawnSystem::PokeballRespawnSystem()
    : System_Internal{ &PokeballRespawnSystem::UpdateComp }
{
}

void PokeballRespawnSystem::UpdateComp(PokeballComponent& comp)
{
    if (!comp.GetIsThrown())
        return;
    if (comp.GetTimeInAir() < 0.8f || ecs::GetEntityTransform(&comp).GetWorldPosition().y > -5.0f)
        return;

    ecs::DeleteEntity(ecs::GetEntity(&comp));
    PrefabManager::LoadPrefab("pokeball");

    // If a target doesn't exist, load one
    if (ecs::GetCompsActiveBegin<PositionRandomizerComponent>() == ecs::GetCompsEnd<PositionRandomizerComponent>())
        PrefabManager::LoadPrefab("target");
}

PokeballKeepFrontSystem::PokeballKeepFrontSystem()
    : System_Internal{ &PokeballKeepFrontSystem::UpdateComp }
    , yaw{}
{
}

bool PokeballKeepFrontSystem::PreRun()
{
    auto gyroCamIter{ ecs::GetCompsActiveBegin<GyroCameraComponent>() };
    if (gyroCamIter == ecs::GetCompsEnd<GyroCameraComponent>())
        return false;

    yaw = gyroCamIter.GetEntity()->GetTransform().GetWorldRotation().y;
    return true;
}

void PokeballKeepFrontSystem::UpdateComp(PokeballComponent& comp)
{
    if (comp.GetIsThrown())
        return;

    Transform& transform{ ecs::GetEntityTransform(&comp) };
    transform.SetWorldRotation(Vec3{ 0.0f, yaw, 0.0f });
    float radianYaw{ math::ToRadians(yaw) };
    transform.SetWorldPosition(Vec3{ sinf(radianYaw), 0.0f, cosf(radianYaw) });
}