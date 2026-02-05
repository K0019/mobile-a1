function OnButtonClicked(entity)
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
    Magic.TimeScale = 0;
    Magic.LoadSceneAdditive("scenes/pausemenu.scene");
end