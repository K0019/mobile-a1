function update(entity)
	if Magic.GetButtonDown("Pause") then
        Magic.TimeScale = 0
        Magic.LoadSceneAdditive("scenes/PauseMenu.scene")
    end
end