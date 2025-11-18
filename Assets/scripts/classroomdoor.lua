local opening = true
local moving = true
local openingDistance = 70.0
local openSpeed = 35
local currentOpeningDistance = 0.0

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
    local transform = entity.transform
    local localPos = transform.worldPosition

    Magic.AudioManager.PlaySound("kalimba",localPos)
end

function update(entity)
    local prevOpenDistance = currentOpeningDistance;
    local transform = entity.transform
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