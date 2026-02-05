function OnButtonClicked(entity)
    Magic.AudioManager.StopAllSounds()
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
    Magic.TransitionScene("scenes/tutorialscene.scene");
end