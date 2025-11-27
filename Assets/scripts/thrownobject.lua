local speed = 10.0
local lifeTime = 0.5
local thisEntity;

function start(entity)
    thisEntity = entity
    local entityReferenceComp = entity:GetEntityReferenceHolderComponent()
    if entityReferenceComp:Exists() then
        local targetEntity = entityReferenceComp:GetEntityReference(0)
        local targetPos = targetEntity.transform.worldPosition
        targetPos.y = targetPos.y + 1.0
        local currPos = entity.transform.worldPosition
        local dir = currPos:Direction(targetPos)
        dir = dir:Normalized()
        dir = dir:Scale(speed)

        local physicsComp = entity:GetPhysicsComp()
        if physicsComp:Exists() then
            physicsComp.linearVelocity = Magic.Vec3(dir.x, dir.y, dir.z)
        end
    end
end

function update(entity)
    lifeTime = lifeTime - Magic:DeltaTime()
    if lifeTime < 0.0 then
        entity:Destroy()
    end
end

function OnTriggerStay(entity)
    local playerComp = entity:GetPlayerMovementComponent()
    if playerComp:Exists() then 
        thisEntity:Destroy()
    end
end