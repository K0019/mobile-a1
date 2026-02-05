local targetEntity
function start(entity)
    local eRefComp = entity:GetEntityReferenceHolderComponent();
    if(eRefComp:Exists()) then
        targetEntity = eRefComp:GetEntityReference(0);
    else
        Magic.Log(Magic.LogLevel.info, "No Entity Reference Holder Component on cheatyteleport!");
    end
end

function OnTriggerEnter(entity)
    -- Handle transform changes
    local worldPos = targetEntity.transform.worldPosition
    entity.transform.worldPosition = worldPos
end