-- How often to play the sound
local footstepInterval = 0.35

-- How often to play the sound
local velocityThreshold = 0.25

-- Whether we're indoors or outdoors
local footstepState = "indoors"


local currentFootstepInterval = 0.0
function setStateOutdoors()
    footstepState = "outdoors"
end

function setStateIndoors()
    footstepState = "indoors"
end

function update(entity)
    -- Sanity checking for character comp and stun time
    local charComp = entity:GetCharacterMovementComponent()
    if not charComp:Exists() then
        return
    end
    if charComp.currentStunTime > 0.0 then
        return
    end

    -- Sanity for Physics Comp
    local physicsComp = entity:GetPhysicsComp();
    if not physicsComp:Exists() then
        return
    end

    local velocity = physicsComp.linearVelocity
    velocity.y = 0.0

    currentFootstepInterval = currentFootstepInterval-Magic.DeltaTime()
    if(currentFootstepInterval<=0.0) then
        if(velocity:LengthSqr()>0.5) then
            currentFootstepInterval = footstepInterval
            if footstepState == "indoors" then
                Magic.AudioManager.PlaySound3DWithVolume("indoor footsteps "..(Magic.Random.RangeInt(0,5)+1), false, entity.transform.worldPosition, Magic.AudioType.SFX, 0.2)
            elseif footstepState == "outdoors" then
                Magic.AudioManager.PlaySound3DWithVolume("outdoor footsteps "..(Magic.Random.RangeInt(0,5)+1), false, entity.transform.worldPosition, Magic.AudioType.SFX, 0.3)
            end
        end
    end
end
--