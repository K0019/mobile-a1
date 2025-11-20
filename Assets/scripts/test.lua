function start(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        Magic.Log(Magic.LogLevel.info, nameComp.name)
        Magic.Log(Magic.LogLevel.info, "HEHE HAHA IS WORKING =================================================")

    end
    local audioSourceComp = entity:GetAudioSourceComponent();
    if audioSourceComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start AUDIO SOURCE =================================================")
        Magic.Log(Magic.LogLevel.info, audioSourceComp.minDistance)
        Magic.Log(Magic.LogLevel.info, audioSourceComp.maxDistance)
        Magic.Log(Magic.LogLevel.info, audioSourceComp.dopplerScale)
        Magic.Log(Magic.LogLevel.info, audioSourceComp.distanceFactor)
        Magic.Log(Magic.LogLevel.info, audioSourceComp.rolloffScale)
        Magic.Log(Magic.LogLevel.info, audioSourceComp.audioFile)
        Magic.Log(Magic.LogLevel.info, tostring(audioSourceComp.isPlaying))
        audioSourceComp:Play(Magic.AudioType.SFX)
        Magic.Log(Magic.LogLevel.info, "END AUDIO SOURCE =================================================")

    end

    local cameraComp = entity:GetCameraComponent();
    if cameraComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start CAMERA  =================================================")
        Magic.Log(Magic.LogLevel.info, tostring(cameraComp.active))
        Magic.Log(Magic.LogLevel.info, cameraComp.zoom)
        cameraComp.active = false
        Magic.Log(Magic.LogLevel.info, tostring(cameraComp.active))
        Magic.Log(Magic.LogLevel.info, "END CAMERA  =================================================")

    end

    local lightBlinkComp = entity:GetLightBlinkComponent();
    if lightBlinkComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start lightBlinkComp  =================================================")
        Magic.Log(Magic.LogLevel.info, lightBlinkComp.minAlpha)
        lightBlinkComp.minAlpha = 0.1
        Magic.Log(Magic.LogLevel.info, lightBlinkComp.minAlpha)
        local  result = lightBlinkComp:AddTimeElapsed(0.016) 
        Magic.Log(Magic.LogLevel.info, "Blink intensity=" .. result.x .. " radius=" .. result.y)
        Magic.Log(Magic.LogLevel.info, "END lightBlinkComp  =================================================")
    end
    local lightComp = entity:GetLightComponent();
    if lightComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start lightComp  =================================================")
        Magic.Log(Magic.LogLevel.info, lightComp.intensity)
        Magic.Log(Magic.LogLevel.info, lightComp.type)
        lightComp.type = Magic.LightType.Spot
        Magic.Log(Magic.LogLevel.info, lightComp.type)
        Magic.Log(Magic.LogLevel.info, "END lightComp  =================================================")
    end

    local boxCollisionComp = entity:GetBoxColliderComp();
    if boxCollisionComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start boxCollisionComp  =================================================")
        Magic.Log(Magic.LogLevel.info, boxCollisionComp.center.x)
        Magic.Log(Magic.LogLevel.info, boxCollisionComp.size.x)
        Magic.Log(Magic.LogLevel.info, "END boxCollisionComp  =================================================")
    end

    local physicsComp = entity:GetPhysicsComp();
    if physicsComp:Exists() then
        --Slightly diff from original physics comp in engine
        Magic.Log(Magic.LogLevel.info, "Start GetPhysicsComp  =================================================")
        -- flags (bools)
        Magic.Log(Magic.LogLevel.info, "isKinematic   = " .. tostring(physicsComp.isKinematic))
        Magic.Log(Magic.LogLevel.info, "useGravity    = " .. tostring(physicsComp.useGravity))
        Magic.Log(Magic.LogLevel.info, "lockRotationX = " .. tostring(physicsComp.lockRotationX))
        Magic.Log(Magic.LogLevel.info, "lockRotationY = " .. tostring(physicsComp.lockRotationY))
        Magic.Log(Magic.LogLevel.info, "lockRotationZ = " .. tostring(physicsComp.lockRotationZ))
        Magic.Log(Magic.LogLevel.info, "enabled       = " .. tostring(physicsComp.enabled))
        -- nudge linear velocity a bit using AddLinearVelocity
        physicsComp:AddLinearVelocity(Magic.Vec3(1.0, 0.0, 0.0))   -- +X impulse
        -- set angular velocity directly
        physicsComp.angularVelocity = Magic.Vec3(0.0, 90.0, 0.0)
        local lv2 = physicsComp.linearVelocity
        Magic.Log(Magic.LogLevel.info,string.format("linearVelocity (after) = (%.3f, %.3f, %.3f)", lv2.x, lv2.y, lv2.z))
        local av2 = physicsComp.angularVelocity
        Magic.Log(Magic.LogLevel.info,string.format("angularVelocity (after) = (%.3f, %.3f, %.3f)", av2.x, av2.y, av2.z))
        Magic.Log(Magic.LogLevel.info, "isKinematic   (after) = " .. tostring(physicsComp.isKinematic))
        Magic.Log(Magic.LogLevel.info, "useGravity    (after) = " .. tostring(physicsComp.useGravity))
        Magic.Log(Magic.LogLevel.info, "END GetPhysicsComp  =================================================")
    end

    local charComp = entity:GetCharacterMovementComponent()
    if charComp and charComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start charComp  =================================================")

        -- read properties
        local mv = charComp.movementVector
        Magic.Log(Magic.LogLevel.info,
            string.format("movementVector = (%.3f, %.3f)", mv.x, mv.y))

        Magic.Log(Magic.LogLevel.info, "moveSpeed           = " .. tostring(charComp.moveSpeed))
        Magic.Log(Magic.LogLevel.info, "rotateSpeed         = " .. tostring(charComp.rotateSpeed))
        Magic.Log(Magic.LogLevel.info, "stunTimePerHit      = " .. tostring(charComp.stunTimePerHit))
        Magic.Log(Magic.LogLevel.info, "groundFriction      = " .. tostring(charComp.groundFriction))
        Magic.Log(Magic.LogLevel.info, "dodgeCooldown       = " .. tostring(charComp.dodgeCooldown))
        Magic.Log(Magic.LogLevel.info, "dodgeDuration       = " .. tostring(charComp.dodgeDuration))
        Magic.Log(Magic.LogLevel.info, "dodgeSpeed          = " .. tostring(charComp.dodgeSpeed))
        Magic.Log(Magic.LogLevel.info, "currentStunTime     = " .. tostring(charComp.currentStunTime))
        Magic.Log(Magic.LogLevel.info, "currentDodgeTime    = " .. tostring(charComp.currentDodgeTime))
        Magic.Log(Magic.LogLevel.info, "currentDodgeCooldown= " .. tostring(charComp.currentDodgeCooldown))

        -- tweak movement vector & speed
        charComp.movementVector = Magic.Vec2(1.0, 0.0)      -- move right
        charComp.moveSpeed      = charComp.moveSpeed + 1.0

        -- try some behaviours
        charComp:Dodge(Magic.Vec2(1.0, 0.0))                -- dodge to the right
        charComp:RotateTowards(Magic.Vec2(0.0, 1.0))        -- face "up"
        charComp:Attack()

        Magic.Log(Magic.LogLevel.info, "END charComp  =================================================")
    end

    local grabComp = entity:GetGrabbableItemComponent()
    if grabComp and grabComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Start grabComp  =================================================")

        Magic.Log(Magic.LogLevel.info, "damage = " .. tostring(grabComp.damage))
        Magic.Log(Magic.LogLevel.info, "isHeld = " .. tostring(grabComp.isHeld))

        -- tweak damage a bit
        grabComp.damage = grabComp.damage + 5.0

        Magic.Log(Magic.LogLevel.info, "damage (after) = " .. tostring(grabComp.damage))

        --IDK HOW TEST PROPERLY HERE IT JUST CRASH
        -- -- test Attack: origin/direction as Vec3
        -- local origin    = Magic.Vec3(0.0, 0.0, 0.0)
        -- local direction = Magic.Vec3(1.0, 0.0, 0.0)

        -- grabComp:Attack(origin, direction)

        Magic.Log(Magic.LogLevel.info, "END grabComp  =================================================")
    end

    local health = entity:GetHealthComponent()
if health and health:Exists() then
    Magic.Log(Magic.LogLevel.info, "=== HealthComponent TEST START ===")

    -- Read current values
    Magic.Log(Magic.LogLevel.info,
        "health    = " .. tostring(health.health))
    Magic.Log(Magic.LogLevel.info,
        "maxHealth = " .. tostring(health.maxHealth))

    Magic.Log(Magic.LogLevel.info,
        "isDead    = " .. tostring(health:IsDead()))
    Magic.Log(Magic.LogLevel.info,
        "fraction  = " .. tostring(health:GetHealthFraction()))

    -- Heal a bit
    health:AddHealth(10.0)
    Magic.Log(Magic.LogLevel.info,
        "after AddHealth(10) -> health = " .. tostring(health.health))

    -- Take damage from some direction
    local dir = Magic.Vec3(1.0, 0.0, 0.0)
    health:TakeDamage(25.0, dir)
    Magic.Log(Magic.LogLevel.info,
        "after TakeDamage(25) -> health = " .. tostring(health.health))

    Magic.Log(Magic.LogLevel.info,
        "isDead (after) = " .. tostring(health:IsDead()))
    Magic.Log(Magic.LogLevel.info,
        "fraction(after) = " .. tostring(health:GetHealthFraction()))

    Magic.Log(Magic.LogLevel.info, "=== HealthComponent TEST END ===")
end

local layerComp = entity:GetEntityLayerComponent()
if layerComp and layerComp:Exists() then
    Magic.Log(Magic.LogLevel.info, "=== EntityLayer TEST START ===")

    Magic.Log(
        Magic.LogLevel.info,
        "Current layer (int) = " .. tostring(layerComp.layer)
    )

    -- Change it to PLAYER
    layerComp.layer = Magic.EntityLayer.Player
    Magic.Log(
        Magic.LogLevel.info,
        "New layer (int) = " .. tostring(layerComp.layer)
    )

    Magic.Log(Magic.LogLevel.info, "=== EntityLayer TEST END ===")
end


    --Magic.TestFunction(entity);
    --print(entity.transform.localPosition.x)
end

function OnTriggerEnter(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Added")
end

function OnTriggerStay(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Staying")
end

function OnTriggerExit(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Exited")
end

function OnCollisionEnter(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Added")
end

function OnCollisionStay(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Staying")
end

function OnCollisionExit(entity)
    Magic.Log(Magic.LogLevel.info, "Contact Exited")
end