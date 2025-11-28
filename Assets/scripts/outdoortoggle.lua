function OnCollisionEnter(entity)
    local scriptComp = entity:GetScriptComponent()
    if scriptComp:Exists() then
        scriptComp:CallScriptFunction("setStateOutdoors")
    end
end