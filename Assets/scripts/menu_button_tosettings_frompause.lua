function OnButtonClicked(entity)
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
    Magic.LoadSceneAdditive("scenes/settings scene.scene");
    Magic.UnloadScene("pausemenu");
end