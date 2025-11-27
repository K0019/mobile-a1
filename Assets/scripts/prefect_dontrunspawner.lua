-- The time between each "Ground Piece" that gets spawned
local spawnInterval = 0.05
-- The distance between each "Ground Piece" that gets spawned
local spawnGap = 0.95
-- The number of "Ground Piece"s to spawn
local spawnCount = 30

-- The current position on this iteration
local currentPosition = Magic.Vec3(0.0,0.0,0.0)

-- Direction that the objects spawn in
local direction = Magic.Vec3(1.0,0.0,0.0)

local currentSpawnInterval = 0.0

function start(entity)
    currentPosition = entity.transform.worldPosition
end

function setDirection(dir)
    direction = dir:Normalized()
end

function update(entity)
    -- Cooldown passed
    if currentSpawnInterval <= 0.0 then

        -- Reset currentSpawnInterval
        currentSpawnInterval = spawnInterval

        -- Decrement spawnCount
        spawnCount = spawnCount - 1

        -- Move once in the direction, multiplied by spawnGap
        local dirToMove = direction:Scale(spawnGap)
        currentPosition = currentPosition:Add(dirToMove)

        -- Spawn the "Ground Piece"
        local newGroundPiece = Magic.PrefabManager.LoadPrefab("prefect_dontrunpiece")
        
        local pieceTransform = newGroundPiece.transform
        local transform = entity.transform
                
        -- Lower the object below the floor
        local verticalOffset = (pieceTransform.worldScale.y / 2) + 0.05 -- extra 0.05 buffer
        currentPosition.y = currentPosition.y - verticalOffset 

        pieceTransform.worldPosition = currentPosition
        pieceTransform.worldRotation = transform.worldRotation

        currentPosition.y = currentPosition.y + verticalOffset -- undo offset


        -- No more to spawn, delete this entity
        if spawnCount <= 0 then
            entity:Destroy()
        end
    end

	currentSpawnInterval = currentSpawnInterval - Magic.DeltaTime()
end