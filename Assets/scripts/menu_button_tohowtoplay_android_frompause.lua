function OnButtonClicked(entity)
    if (Magic.IsAndroid()) then
        Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
        Magic.LoadSceneAdditive("scenes/how to play android.scene");
        Magic.UnloadScene("pausemenu");
    end
end