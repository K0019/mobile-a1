local thisEntity
local volumeStep = 0.1

function OnButtonPressed()
	local currentVolume = GetCurrentVolume()
	local newVolume = math.min(currentVolume + volumeStep, 1.0)
	Magic.AudioManager.SetCategoryVolume(Magic.AudioType.BGM, newVolume)
	UpdateSliderVisual(newVolume)
end

function GetCurrentVolume()
	if thisEntity and thisEntity:GetSliderComponent():Exists() then
		return thisEntity:GetSliderComponent().progress
	end
	return 0.5
end

function UpdateSliderVisual(newVolume)
	if thisEntity and thisEntity:GetSliderComponent():Exists() then
		local sliderComp = thisEntity:GetSliderComponent()
		sliderComp.progress = newVolume
		sliderComp:UpdateVisualPosition()
	end
end

function start(entity)
	thisEntity = entity
end