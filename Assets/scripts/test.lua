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

    --Magic.TestFunction(entity);
    --print(entity.transform.localPosition.x)
end