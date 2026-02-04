function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            --Magic.Log("Player has entered the Buddha trigger area. Removing invincibility.");
            local healthComp = entity:GetHealthComponent();
            if healthComp:Exists() then
                healthComp.invincibleState = 0;
           end
            
        end
    end
end