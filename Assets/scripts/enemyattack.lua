function OnTriggerEnter(entity)
    local characterComp = entity:GetCharacterMovementComponent();
    local playerComp = entity:GetPlayerMovementComponent();
    local healthComp = entity:GetHealthComponent();
    if characterComp:Exists() and playerComp:Exists() and healthComp:Exists() then
        healthComp:TakeDamage(4.0, Magic.Vec3(0.0, 0.0, 0.0));
    end
end