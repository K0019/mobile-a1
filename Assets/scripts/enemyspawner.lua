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
    
    local nameComp = thisEntity:GetNameComponent()
    Magic.Log(Magic.LogLevel.info, nameComp.name)
    if nameComp.name == "startspawner" then
    wavespawn()
    end
end

function proceed()
    local entityContainer = thisEntity:GetEntityReferenceHolderComponent()

    if entityContainer:Exists() then
    local nextObjective = entityContainer:GetEntityReference(0)
    
    Magic.Log(Magic.LogLevel.info, "proceed")

    -- spawn type of enemy based on referenced object name
    local nameComp = nextObjective:GetNameComponent()
    Magic.Log(Magic.LogLevel.info, nameComp.name)

    local scriptComp = nextObjective:GetScriptComponent()
    
    if scriptComp:Exists() then
    
        if nameComp.name == "enemyspawner" then
            Magic.Log(Magic.LogLevel.info, "It's spawnin time")
            scriptComp:CallScriptFunction("wavespawn")
            
        elseif nameComp.name == "Door_Classroom" then
            Magic.Log(Magic.LogLevel.info, "Open sesame")
            scriptComp:CallScriptFunction("open")

        elseif nameComp.name == "winscreen" then
            Magic.Log(Magic.LogLevel.info, "I win")
            --scriptComp:CallScriptFunction("wingame")
        end
    end

end

end

function enemydeath()
    if EnemySpawner.aliveEnemies <= 0 then
        return
    end

    EnemySpawner.aliveEnemies = EnemySpawner.aliveEnemies - 1
    Magic.Log(Magic.LogLevel.info, "Enemy died. Remaining = "..EnemySpawner.aliveEnemies)
    
    if EnemySpawner.aliveEnemies == 0 then 
    Magic.Log(Magic.LogLevel.info, "proceed")
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

             -- spawn type of enemy based on referenced object name
             local nameComp = spawnPosEntity:GetNameComponent()
             Magic.Log(Magic.LogLevel.info, nameComp.name)

             -- load and spawn enemy based on assigned name and pos
             local newEnemy = Magic.PrefabManager.LoadPrefab(nameComp.name)
             local newEnemyTransform = newEnemy.transform
             Magic.Log(Magic.LogLevel.info, "Spawned "..nameComp.name)

             local transform = spawnPosEntity.transform
             local worldPos = transform.worldPosition
             --Magic.Log(Magic.LogLevel.info,string.format("linearVelocity (after) = (%.3f, %.3f, %.3f)", worldPos.x, worldPos.y, worldPos.z))

             newEnemyTransform.worldPosition = worldPos

             -- keep track of enemies spawned
            EnemySpawner.aliveEnemies = EnemySpawner.aliveEnemies + 1
            Magic.Log(Magic.LogLevel.info, "Enemy spawned. Remaining = "..EnemySpawner.aliveEnemies)

            local selfref = newEnemy:GetEntityReferenceHolderComponent()
            if selfref:Exists() then
                selfref:SetEntityReference(0, thisEntity)
            end
        end
    else
        Magic.Log(Magic.LogLevel.info, "ENTITYCONTAINER IS NULL")
    end
end