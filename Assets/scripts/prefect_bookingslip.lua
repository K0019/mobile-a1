-- How rapidly the booking slip spins as it flies
local rotationSpeed = 360.0
-- How close to the target before it switches to "Dive" phase
local diveDistance = 1.5 * 1.5
-- How fast it moves
local moveSpeed = 3.0
-- How fast it dives
local diveSpeed = 3.0
-- How long to chase for befor automatically diving
local lifeTime = 3.0

local thisEntity;
local targetEntity;
function start(entity)
    thisEntity = entity
    local entityContainer = thisEntity:GetEntityReferenceHolderComponent()

    if entityContainer:Exists() then
        targetEntity = entityContainer:GetEntityReference(0)
    end
end

function update(entity)
    local transform = entity.transform
    local worldPos = transform.worldPosition
    local worldRot = transform.worldRotation

    worldRot.y = worldRot.y + rotationSpeed * Magic:DeltaTime()

    if(worldRot.y >= 360.0) then
        worldRot.y = worldRot.y - 360.0  
    end

    transform.worldRotation = worldRot

    local targetTransform = targetEntity.transform
    local targetPos = targetTransform.worldPosition

    -- We need this line otherwise the slip will only target your feet...
    -- Foot origin is fun
    targetPos.y = targetPos.y + 0.8

    local moveDirection = worldPos:Direction(targetPos)
    local distanceSqr = moveDirection:LengthSqr() 

    -- Instantly drop the booking slip if it's close to the player
    if (distanceSqr < diveDistance) then
        lifeTime = 0.0
    end


    moveDirection = moveDirection:Normalized()

    lifeTime = lifeTime - Magic:DeltaTime()

    -- If no more lifetime, auto dive
    if(lifeTime< 0.0)   then
    
        worldPos.y = worldPos.y - diveSpeed*Magic:DeltaTime()
    end
    -- Auto destroy if it's been falling for too long, prevent potential excess entities
    if(lifeTime < -10.0)then
    
        entity:Destroy()
    end

    worldPos.x = worldPos.x + (moveDirection.x * moveSpeed * Magic:DeltaTime())
    worldPos.y = worldPos.y + (moveDirection.y * moveSpeed * Magic:DeltaTime())
    worldPos.z = worldPos.z + (moveDirection.z * moveSpeed * Magic:DeltaTime())

    transform.worldPosition = worldPos
end

function OnTriggerStay(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if nameComp.name == "Player" or lifeTime<=0.0 then
            local newExplosion = Magic.PrefabManager.LoadPrefab("explosion")
            
            local explosionTransform = newExplosion.transform
            
            local transform = entity.transform
            local worldPos = transform.worldPosition
            
            explosionTransform.worldPosition = worldPos

            thisEntity:Destroy()
        end
    end
end