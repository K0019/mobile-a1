function update(entity)
	local spriteComp = entity:GetSpriteComponent()
	if spriteComp:Exists() then
		local currColor = spriteComp.color
		if Magic.EngineIsFullscreen then
			currColor.w = 1.0
		else
			currColor.w = 0.0
		end
		spriteComp.color = currColor
	end
end