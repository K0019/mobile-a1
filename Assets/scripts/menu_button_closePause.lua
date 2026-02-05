function OnButtonClicked(entity)
    Magic.AudioManager.PlaySoundWithVolume("ui button", false, Magic.AudioType.SFX, 0.6)
	Magic.TimeScale = 1
	Magic.UnloadScene("pausemenu");
end