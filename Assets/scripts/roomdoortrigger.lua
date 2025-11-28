local thisEntity
local opensDoor = false
local enemySpawnDelay = 3.0
local spawnIncoming = false
function start(entity)
    thisEntity = entity;

    local nameComp = entity:GetNameComponent()

    if(nameComp.name == "DoorTrigger_Open") then
        opensDoor = true
    end

    -- Delete the debug RenderComp on this entity
    local renderComp = entity:GetRenderComponent()
    if(renderComp:Exists()) then
        renderComp:Remove() 
    end

end

function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            --Magic.Log(Magic.LogLevel.info, nameComp.name)
            local entityContainer = thisEntity:GetEntityReferenceHolderComponent()

            if entityContainer:Exists() then
                --Magic.Log(Magic.LogLevel.info, "Yay?")
                local doorEntity = entityContainer:GetEntityReference(0)
                local scriptComp = doorEntity:GetScriptComponent()

                if(opensDoor) then
                    scriptComp:CallScriptFunction("open")
                else
                    scriptComp:CallScriptFunction("close")
                end

                spawnIncoming = true;
            end
        end
    end

end

function update(entity)
    if spawnIncoming then
        if enemySpawnDelay <= 0.0 then
            local entityContainer = entity:GetEntityReferenceHolderComponent()

            if entityContainer:Exists() then
            -- Call relevant functions on enemy wave spawner etc here!
                if entityContainer:GetSize() >= 2 then
                    local spawnerEntity = entityContainer:GetEntityReference(1)
                    local spawnerScriptComp = spawnerEntity:GetScriptComponent()

                    if spawnerScriptComp:Exists() then
                        --Magic.Log(Magic.LogLevel.info, "It's spawnin time")
                        spawnerScriptComp:CallScriptFunction("wavespawn")
                    end
                end
            end

            entity:Destroy()
        end
        enemySpawnDelay = enemySpawnDelay - Magic:DeltaTime()
    end
end