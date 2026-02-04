local thisEntity

function OnSliderValueFinalized()
	Magic.AudioManager.SetCategoryVolume(Magic.AudioType.BGM, thisEntity:GetSliderComponent().progress)
end

function start(entity)
    thisEntity = entity
end