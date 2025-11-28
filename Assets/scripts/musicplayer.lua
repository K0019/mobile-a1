local musicVolume = 0.1
local ambienceVolume = 0.15

function start(entity)

    
    if _G.musicHandle == nil then
        Magic.Log(Magic.LogLevel.info, "Music Handle is NULL!")
    else
        Magic.Log(Magic.LogLevel.info, "Music Handle At Delete: ".._G.musicHandle)
        Magic.AudioManager.StopSound(_G.musicHandle)
        Magic.AudioManager.StopSound(_G.ambienceHandle)
    end

    _G.musicHandle = Magic.AudioManager.PlaySoundWithVolume("purrfectputt-music-gameplayloop1",true,musicVolume)
    _G.ambienceHandle = Magic.AudioManager.PlaySoundWithVolume("school ambience heavy voices_loop",true,ambienceVolume)

    Magic.Log(Magic.LogLevel.info, "Music Handle: ".._G.musicHandle)
end