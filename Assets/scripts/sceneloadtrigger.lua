function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            Magic.TransitionScene("scenes/BossScene.scene")
            -- Magic.TransitionScene("scenes/defaultscene.scene")
        end
    end
end