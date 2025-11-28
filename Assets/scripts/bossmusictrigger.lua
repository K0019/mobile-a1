local thisEntity;
function start(entity)
    thisEntity = entity    
end

function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if not nameComp:Exists() then
        Magic.Log(Magic.LogLevel.info, "NO NAME!!!")
        return
    end
    if not(nameComp.name  == "Player") then
        return
    end
    local entityContainer = thisEntity:GetEntityReferenceHolderComponent()
    if not entityContainer:Exists() then
        return
    end
    
    local musicPlayerReference = entityContainer:GetEntityReference(2)
    local scriptComp = musicPlayerReference:GetScriptComponent()
    if scriptComp:Exists() then
        scriptComp:CallScriptFunction("setBossMusic")
        Magic.Log(Magic.LogLevel.info, "Setting boss music!")
    end
end
