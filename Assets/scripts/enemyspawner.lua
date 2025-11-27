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
    local scriptComp = nextObjective:GetScriptComponent()
          if scriptComp:Exists() then
          Magic.Log(Magic.LogLevel.info, "It's spawnin time")
          scriptComp:CallScriptFunction("wavespawn")
          
          else --open door or end game
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

        local numToSpawn = entityContainer:GetSize();
        Magic.Log(Magic.LogLevel.info, "Size: "..numToSpawn - 1)

        -- for each assigned reference after the first
        for i = 1, numToSpawn - 1 do
             local spawnPosEntity = entityContainer:GetEntityReference(i)

             -- spawn position based on referenced object 1 - however many
             local worldPos = spawnPosEntity.worldPosition
             Magic.Log(Magic.LogLevel.info, "Spawn Pos"..worldPos)

             -- spawn type of enemy based on referenced object name
             local nameComp = spawnPosEntity:GetNameComponent()
             Magic.Log(Magic.LogLevel.info, nameComp.name)

             -- load and spawn enemy based on assigned name and pos
             local newEnemy = Magic.PrefabManager.LoadPrefab(nameComp.name)
             newEnemy.transform.worldPosition = worldPos

             -- keep track of enemies spawned
            EnemySpawner.aliveEnemies = EnemySpawner.aliveEnemies + 1
            
            local scriptComp = newEnemy:GetScriptComponent()
            if scriptComp:Exists then
            --scriptComp.OnHealthDepleted = function()
                    --enemydeath(newEnemy)
            end

        end
    else
        Magic.Log(Magic.LogLevel.info, "ENTITYCONTAINER IS NULL")
    end
end