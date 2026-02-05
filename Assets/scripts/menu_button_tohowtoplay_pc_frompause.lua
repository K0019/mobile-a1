function OnButtonClicked(entity)
    if (Magic.IsAndroid()) then
        return
    end
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
    Magic.LoadSceneAdditive("scenes/how to play scene pc.scene");
    Magic.UnloadScene("pausemenu");
end