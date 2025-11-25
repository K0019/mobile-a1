-- How much to heal when picking this up
local healAmount = 15.0

-- Needed
local thisEntity

function start(entity)
	thisEntity = entity
end

function OnCollisionEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if(nameComp.name == "Player") then
            local healthComp = entity:GetHealthComponent()

            -- Attempt heal
            if healthComp:Exists() then
                Magic.Log(Magic.LogLevel.info, "Healing Player!")

                -- Heal a bit
                healthComp:AddHealth(healAmount)
                Magic.Log(Magic.LogLevel.info,
                    "after AddHealth(10) -> health = " .. tostring(health.health))
            end

            -- Delete self
            entity:Destroy()
        end
    end
end

