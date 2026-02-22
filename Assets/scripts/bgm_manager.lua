-- BGM Manager Script
-- Attach this script to any persistent entity in the scene.
-- It will play a looping BGM track when the entity starts.
--
-- To use your own music, replace "placeholder_bgm" below with
-- the name of your audio asset (the filename without extension,
-- as registered in the AssetManager).

-- ============================================================
-- PLACEHOLDER: Change this string to your actual BGM asset name
local BGM_SOUND_NAME = "bgm(sergio's magic dustbin)"
-- ============================================================

local bgmHandle = nil

function start(entity)
    -- Play the BGM on loop using the AudioManager
    -- Parameters: sound name, looping, audio category (BGM)
    bgmHandle = Magic.AudioManager.PlaySound(BGM_SOUND_NAME, true, Magic.AudioType.BGM)
    Magic.Log(Magic.LogLevel.info, "[BGM] Started playing: " .. BGM_SOUND_NAME)
end

function onDestroy(entity)
    -- Stop the BGM when this entity is destroyed (e.g. scene unload)
    if bgmHandle ~= nil then
        Magic.AudioManager.StopSound(bgmHandle)
        bgmHandle = nil
        Magic.Log(Magic.LogLevel.info, "[BGM] Stopped playing: " .. BGM_SOUND_NAME)
    end
end
