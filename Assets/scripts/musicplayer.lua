local musicVolume = 0.2
local ambienceVolume = 0.15

local musicHandle
local ambienceHandle

local musicSwitchCountdown = 3.0
local switchingMusic = false
local switchedMusic = false
function start(entity)
    musicHandle = Magic.AudioManager.PlaySoundWithVolume("purrfectputt-music-gameplayloop1",true,musicVolume)
    ambienceHandle = Magic.AudioManager.PlaySoundWithVolume("school ambience heavy voices_loop",true,ambienceVolume)

    Magic.Log(Magic.LogLevel.info, "Music Handle: "..musicHandle)
end

function onDestroy(entity)
    Magic.AudioManager.StopSound(musicHandle)
    Magic.AudioManager.StopSound(ambienceHandle)
end

function setBossMusic()
    Magic.Log(Magic.LogLevel.info, "SET MUSIC MESSAGE RECEIVED!!!")
    Magic.AudioManager.StopSound(ambienceHandle)
    ambienceHandle=0

    Magic.AudioManager.FadeOutSound(musicHandle,2.0)
    switchingMusic =true
end

function update(entity)
    if switchedMusic then
        return
    end

    if switchingMusic then
        musicSwitchCountdown = musicSwitchCountdown - Magic.DeltaTime()
        if musicSwitchCountdown <= 0.0 then
            musicHandle = Magic.AudioManager.PlaySoundWithVolume("cut&pastemenumusic",true,musicVolume)
            switchedMusic = true
        end
    end
end
