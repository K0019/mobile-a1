function update(entity)
	if Magic.GetButtonDown("Pause") then
		Magic.TimeScale = 1
		Magic.UnloadScene("pausemenu");
    end
end