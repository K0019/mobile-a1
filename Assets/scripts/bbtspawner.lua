local spawnTime = 1.0
local positionOffset = Magic.Vec3(0.0,1.0,0.0)

local currentSpawnTime = 0.0
local bbtObject

local thisEntity
function start(entity)
    thisEntity = entity
end

function update(entity)
    local needSpawn = false
    if bbtObject == nil then
        needSpawn = true
    end

    if not needSpawn then
        if not bbtObject:IsValid() then
            needSpawn = true
        end
    end

    if needSpawn then
        if currentSpawnTime <= 0.0 then
            bbtObject = Magic.PrefabManager.LoadPrefab("bbt")


            local thisTransform = thisEntity.transform
            local bbtTransform = bbtObject.transform
            
            local spawnPos = thisTransform.worldPosition

            spawnPos = spawnPos:Add(positionOffset)

            bbtTransform.worldPosition = spawnPos;

            currentSpawnTime = spawnTime
        end
        currentSpawnTime = currentSpawnTime - Magic.DeltaTime()
    end
end