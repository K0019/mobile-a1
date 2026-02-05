function OnButtonClicked(entity)
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
    Magic.LoadSceneAdditive("scenes/pausemenu.scene");
    Magic.UnloadScene("how to play android");
end