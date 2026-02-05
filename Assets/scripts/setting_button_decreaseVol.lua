local thisEntity
local volumeStep = 0.1

function OnButtonPressed()
	local currentVolume = GetCurrentVolume()
	local newVolume = math.max(currentVolume - volumeStep, 0.0)
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
		thisEntity:GetSliderComponent().progress = newVolume
	end
end

function start(entity)
	thisEntity = entity
end