-- The amount of time it takes for the explosion to reach max size, then disappear
local explosionGrowthTime = 0.6
local currentExplosionGrowthTime = explosionGrowthTime
local thisEntity;
-- Impt stats
local maxExplosionSize = 4.0
local damage = 20.0

function setSize(size)
    maxExplosionSize=size  
end

function setDamage(damage)
    maxExplosionSize=size  
end

function start(entity)
    thisEntity = entity
end

function update(entity)
	currentExplosionGrowthTime = currentExplosionGrowthTime - Magic.DeltaTime()

    -- Current size increases linearly
    local currentSize = (explosionGrowthTime-currentExplosionGrowthTime)/explosionGrowthTime*maxExplosionSize

    local transform = entity.transform
    local worldScale = transform.worldScale

    -- Scale uniformly!
    worldScale.x = currentSize
    worldScale.y = currentSize
    worldScale.z = currentSize

    -- Destroy this entity if we're at the end of the lifetime
    if(currentExplosionGrowthTime<=0.0) then
        entity:Destroy()
    end
    transform.worldScale = worldScale
end

function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            Magic.Log(Magic.LogLevel.info, nameComp.name)
            
            local healthComp = entity:GetHealthComponent()
            
            healthComp:TakeDamage(damage,Magic.Vec3(0.0, 0.0, 0.0))
        end
    end

end