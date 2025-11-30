function OnTriggerEnter(entity)
    -- Handle transform changes
    local worldPos = entity.transform.worldPosition
    worldPos.x = worldPos.x - 3.0
    entity.transform.worldPosition = worldPos
end