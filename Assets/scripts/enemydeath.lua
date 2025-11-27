-- One in <this> chance to drop that boba mmmmmmmmmmmmmmmmm
local randomChance = 1
local positionOffset = Magic.Vec3(0.0,1.0,0.0)
local minRandomVelocity = Magic.Vec3(-1.0,-1.0,-1.0)
local maxRandomVelocity = Magic.Vec3(1.0,1.0,1.0)

local thisEntity

function start(entity)
    thisEntity = entity
end

function OnHealthDepleted()
    -- Play death sound
    Magic.AudioManager.PlaySound3D("enemy female death "..(Magic.Random.RangeInt(0,3)+1),false,thisEntity.transform.worldPosition)

    -- Randomly drop a BBT
    if Magic.Random.DiceRoll(randomChance) then
        local newBBT = Magic.PrefabManager.LoadPrefab("bbt")


        local thisTransform = thisEntity.transform
        local bbtTransform = newBBT.transform
        
        local spawnPos = thisTransform.worldPosition

        spawnPos = spawnPos:Add(positionOffset)

        bbtTransform.worldPosition = spawnPos;

        -- Add a random kick to the boba
        local physicsComp = newBBT:GetPhysicsComp();
        if physicsComp:Exists() then
            local randomVelocity = Magic.Random.RangeVec3(minRandomVelocity,maxRandomVelocity)
            physicsComp:AddLinearVelocity(randomVelocity)
        end

    end
end

function OnDamaged(amount,direction)
    -- Play death sound
    Magic.AudioManager.PlaySound3D("enemy female hurt "..(Magic.Random.RangeInt(0,4)+1),false,thisEntity.transform.worldPosition)
end