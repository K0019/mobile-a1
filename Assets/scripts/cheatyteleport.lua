function OnTriggerEnter(entity)
	    -- Handle transform changes
    local thisTransform = entity.transform;
    local worldPos = thisTransform.worldPosition
    worldPos.x = worldPos.x - 3.0
    thisTransform.worldPosition = worldPos
end