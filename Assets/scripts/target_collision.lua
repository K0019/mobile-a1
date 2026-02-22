-- ============================================================
-- PLACEHOLDER: Change this string to your actual hit SFX asset name
local HIT_SFX_NAME = "nice shot! wii sports - quicksounds.com"
-- ============================================================

function OnTriggerEnter(targetEntity, ballEntity)
    local ballComp = ballEntity:GetPokeballComponent();
    if ballComp:Exists() then
        -- Play the hit sound effect (one-shot, no loop, SFX category)
        Magic.AudioManager.PlaySound(HIT_SFX_NAME, false, Magic.AudioType.SFX)
        Magic.Log(Magic.LogLevel.info, "[SFX] Pokeball hit target!")

        ballComp:OnTargetHit(targetEntity)
    end
end