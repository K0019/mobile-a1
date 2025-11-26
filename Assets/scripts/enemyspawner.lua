local thisEntity

EnemySpawner = {
    aliveEnemies = 0
}

function start(entity)
    -- Delete the debug RenderComp on this entity, makes it invis?
    local renderComp = entity:GetRenderComponent()
    if(renderComp:Exists()) then
        renderComp:Remove() 
    end

	thisEntity = entity;
    
    wavespawn()
end

function proceed()
    local entityContainer = thisEntity:GetEntityReferenceHolderComponent()

    if entityContainer:Exists() then
    local nextObjective = entityContainer:GetEntityReference(0)

        if nextObjective:Exists() then
            local scriptComp = nextObjective:GetScriptComponent()

            if scriptComp:Exists() then
                Magic.Log(Magic.LogLevel.info, "It's spawnin time")
                scriptComp:CallScriptFunction("wavespawn")

            else --open door or end game
            end
        end
    end
end

function enemydeath(entity)
    EnemySpawner.aliveEnemies = EnemySpawner.aliveEnemies - 1
    Magic.Log(Magic.LogLevel.info, "Enemy died. Remaining = "..EnemySpawner.aliveEnemies)
    
    if EnemySpawner.aliveEnemies == 0 then 
    proceed()
    end
end

function wavespawn()
    local entityContainer = thisEntity:GetEntityReferenceHolderComponent()
    
    if entityContainer:Exists() then
        local numToSpawn = 2--entityContainer:GetSize();
        Magic.Log(Magic.LogLevel.info, "Size: "..entityContainer:GetSize())
        -- for i = 1, numToSpawn - 1 do 
        --     Magic.Log(Magic.LogLevel.info, "Spawn Pos")
        --     local spawnPosEntity = entityContainer:GetEntityReference(i)

        --     local worldPos = spawnPosEntity.worldPosition
            
        --     local newEnemy = Magic.PrefabManager.LoadPrefab("enemy")
        --     newEnemy.transform.worldPosition = worldPos
            --EnemySpawner.aliveEnemies = EnemySpawner.aliveEnemies + 1
        --end
        --end
    else
        Magic.Log(Magic.LogLevel.info, "ENTITYCONTAINER IS NULL")
    end
end