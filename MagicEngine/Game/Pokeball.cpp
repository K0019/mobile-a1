#include "Pokeball.h"
#include "Engine/Platform/Android/AndroidInputBridge.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Input.h"
#include "Engine/PrefabManager.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"
#include "core/platform/platform.h"

#include <glm/gtc/quaternion.hpp>

#include "Physics/Physics.h"
#include "Game/Target.h"
#include "Game/GyroCamera.h"
#include "Engine/Audio.h"

#include "Engine/EntityEvents.h"

PokeballComponent::PokeballComponent()
    : isThrown{}
    , launchTime{}
    , spinAngle{}
{
	RandomizeIdleSpin();
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

Vec3 PokeballComponent::GetSpinAxis() const
{
    return spinAxis;
}

float PokeballComponent::GetSpinSpeed() const
{
    return spinSpeed;
}

float PokeballComponent::GetSpinAngle() const
{
    return spinAngle;
}

void PokeballComponent::AccumulateSpin(float dt)
{
    spinAngle += spinSpeed * dt;
    if (spinAngle > 360.0f)
        spinAngle -= 360.0f;
}

void PokeballComponent::RandomizeIdleSpin()
{
	spinAxis = Vec3{ randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f) };
	float len = spinAxis.Length();
	if (len > 0.001f)
		spinAxis = spinAxis * (1.0f / len);
	else
		spinAxis = Vec3{ 0.0f, 1.0f, 0.0f };

	float baseSpeed = randomFloat(90.0f, 270.0f);
	float sign = (randomRange(0, 1) == 0 ? -1.0f : 1.0f);
	spinSpeed = baseSpeed * sign;
}

void PokeballComponent::OnTargetHit(ecs::EntityHandle targetEntity)
{
    CONSOLE_LOG(LEVEL_INFO) << "TARGET HIT!";

    float randomVal = (randomFloat(1.0f, 10.0f) * 10);
    ST<EventsQueue>::Get()->AddEventForNextFrame(Events::Game_ScoreUpdate{ randomVal});

    ecs::DeleteEntity(targetEntity);
    ST<EventsQueue>::Get()->AddEventForNextFrame(Events::Game_NiceThrow{});

    // Optionally play a hit sound from the referenced audio entity (looked up via UID)
    if (hitSoundSource.IsValidReference())
    {
        if (auto audioEntity = static_cast<ecs::EntityHandle>(hitSoundSource))
        {
            if (auto* audioComp = audioEntity->GetComp<AudioSourceComponent>())
            {
                audioComp->Play(AudioType::SFX);
            }
        }
    }

    // Let respawn system handle our ball respawning
    launchTime = -999.0f;
    ecs::GetEntityTransform(this).SetWorldPosition(Vec3{ 0.0f, -999.0f, 0.0f });

	// Optionally play a hit sound from the referenced audio entity (looked up via UID)
	if (hitSoundSource.IsValidReference())
	{
		ecs::EntityHandle audioEntity = hitSoundSource;
		if (audioEntity)
		{
			if (auto* audioComp = audioEntity->GetComp<AudioSourceComponent>())
			{
				audioComp->Play(AudioType::SFX);
			}
		}
	}
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
    if (swipeDir.LengthSqr() <= std::numeric_limits<float>::epsilon() || swipeDir.LengthSqr() > 1e10f)
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

	// Delete old ball and spawn a new one; ensure the new ball gets fresh spin state
	ecs::DeleteEntity(ecs::GetEntity(&comp));
	auto newBall = PrefabManager::LoadPrefab("pokeball");
	if (newBall)
	{
		if (auto* newComp = newBall->GetComp<PokeballComponent>())
		{
			// Re-randomize idle spin so each respawn can spin in a new direction
			newComp->RandomizeIdleSpin();
		}
	}

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

    comp.AccumulateSpin(GameTime::Dt());

    // Build a quaternion for the idle spin around the random axis
    float spinRad = math::ToRadians(comp.GetSpinAngle());
    glm::quat spinQuat = glm::angleAxis(spinRad, glm::normalize(glm::vec3(comp.GetSpinAxis())));

    // Build a quaternion for facing the camera yaw
    glm::quat yawQuat = glm::angleAxis(math::ToRadians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

    // Combine: face the camera, then apply the idle spin
    glm::quat finalQuat = yawQuat * spinQuat;

    // Extract Euler angles using YXZ convention to match Mat4::Set's glm::yawPitchRoll(y,x,z).
    // Using glm::eulerAngles here causes gimbal lock because it uses a different decomposition.
    float extractedY, extractedX, extractedZ;
    glm::extractEulerAngleYXZ(glm::mat4_cast(finalQuat), extractedY, extractedX, extractedZ);
    Vec3 eulerDeg{ math::ToDegrees(extractedX), math::ToDegrees(extractedY), math::ToDegrees(extractedZ) };

    Transform& transform{ ecs::GetEntityTransform(&comp) };
    transform.SetWorldRotation(eulerDeg);
    float radianYaw{ math::ToRadians(yaw) };
    transform.SetWorldPosition(Vec3{ sinf(radianYaw), 0.0f, cosf(radianYaw) });
}