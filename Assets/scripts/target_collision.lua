function OnTriggerEnter(targetEntity, ballEntity)
    local ballComp = ballEntity:GetPokeballComponent();
    if ballComp:Exists() then
        ballComp:OnTargetHit(targetEntity)
    end
end