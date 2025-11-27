-- Damage dealt (per section)
local damage = 10
-- How fast it rises
local riseSpeed = 3.0
-- How fast it rises
local sinkSpeed = 1.0
-- How far it rises/sinks
local riseDistance = 1.5

---------------------------
-- Not to be edited!
local currentHeight = 0.0
local rising = true
local thisEntity
---------------------------

function update(entity)
    thisEntity = entity
    local previousHeight = currentHeight
	if rising then
        currentHeight = currentHeight + (riseSpeed * Magic:DeltaTime())

        -- At the peak, we clamp then start sinking
        if currentHeight >=riseDistance then
            currentHeight = riseDistance
            rising = false
        end
    else
        currentHeight = currentHeight - (sinkSpeed * Magic:DeltaTime())

        -- Once we're done sinking, kill the entity
        if currentHeight <= 0 then
            entity:Destroy()
        end
    end

    -- Handle transform changes
    local thisTransform = entity.transform;
    local worldPos = thisTransform.worldPosition
    
    worldPos.y = worldPos.y + (currentHeight-previousHeight)

    thisTransform.worldPosition = worldPos
end

function OnTriggerEnter(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        if nameComp.name == "Player" then
            local healthComp = entity:GetHealthComponent()          
            healthComp:TakeDamage(damage,Magic.Vec3(0.0, 0.0, 0.0))
        end
    end

end