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
    --Magic.TestFunction(entity);
    --print(entity.transform.localPosition.x)
end