local thisEntity;
local characterEntity;

function start(entity)
    thisEntity = entity
end

function throw(entity)
    local colliderComp = thisEntity:GetBoxColliderComp()
    if colliderComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "Throw")
        colliderComp.isTrigger = true
    end
    characterEntity = entity;
end


function OnTriggerEnter(entity)
    local colliderComp = thisEntity:GetBoxColliderComp()
    local charName = characterEntity:GetNameComponent()
    local entityName = entity:GetNameComponent()
    if charName:Exists() and entityName:Exists() then
        if charName.name ~= entityName.name then
            if colliderComp:Exists() then
                colliderComp.isTrigger = false
            end
            local playerComp = entity:GetPlayerMovementComponent()
            if playerComp:Exists() then
                local healthComp = entity:GetHealthComponent()
                if healthComp:Exists() then
                    local dir = Magic.Vec3(1.0, 0.0, 0.0)
                    healthComp:TakeDamage(25.0, dir)
                end
            end
        end
    end

end