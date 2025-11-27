local thisEntity
local opensDoor = false
function start(entity)
    local nameComp = entity:GetNameComponent()

    if(nameComp.name == "DoorTrigger_Open") then
        opensDoor = true
    end

    -- Delete the debug RenderComp on this entity
    local renderComp = entity:GetRenderComponent()
    if(renderComp:Exists()) then
        renderComp:Remove() 
    end

	thisEntity = entity;
end

function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            Magic.Log(Magic.LogLevel.info, nameComp.name)
            local entityContainer = thisEntity:GetEntityReferenceHolderComponent()

            if entityContainer:Exists() then
                Magic.Log(Magic.LogLevel.info, "Yay?")
                local doorEntity = entityContainer:GetEntityReference(0)
                local scriptComp = doorEntity:GetScriptComponent()

                if(opensDoor) then
                    scriptComp:CallScriptFunction("open")
                else
                    scriptComp:CallScriptFunction("close")
                end

            -- Call relevant functions on enemy wave spawner etc here!

            else
                Magic.Log(Magic.LogLevel.info, "NO COMPONENT ATTACHED!!!")
            end
        end
    end

end