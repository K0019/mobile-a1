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
    --Magic.TestFunction(entity);
    --print(entity.transform.localPosition.x)
end