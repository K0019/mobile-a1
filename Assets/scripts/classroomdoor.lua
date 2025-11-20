local opening = true
local moving = true
local openingDistance = 0.6
local openSpeed = 0.3
local currentOpeningDistance = 0.0
local openingDoorEntity
local closingDoorEntity
function open()
    opening = true
    moving = true

end

function close()
    opening = false
    moving = true
end

function toggle()
    opening = not opening
    moving = true
end

function start(entity)

    local entityContainer = entity:GetEntityReferenceHolderComponent()

    if entityContainer:Exists() then
        Magic.Log(Magic.LogLevel.info, "Yay?")
        openingDoorEntity = entityContainer:GetEntityReference(0)
        closingDoorEntity = entityContainer:GetEntityReference(1)
    else
        Magic.Log(Magic.LogLevel.info, "NO COMPONENT ATTACHED!!!")
    end

    -- Audio plays, direction TBD
    -- Magic.AudioManager.PlaySound("kalimba",localPos)
end

function update(entity)
    local prevOpenDistance = currentOpeningDistance;
    local transform = openingDoorEntity.transform
    local localPos = transform.localPosition

    if(moving) then
        if(opening) then
                currentOpeningDistance = currentOpeningDistance + Magic.DeltaTime() * openSpeed

                if(currentOpeningDistance > openingDistance) then
                    currentOpeningDistance = openingDistance
                moving = false
            end
        else
            currentOpeningDistance = currentOpeningDistance - Magic.DeltaTime() * openSpeed

            if(currentOpeningDistance < 0) then
                currentOpeningDistance = 0
                moving = false
            end
        end
    end

    localPos.z = localPos.z + (currentOpeningDistance - prevOpenDistance)
    
    transform.localPosition = localPos


end