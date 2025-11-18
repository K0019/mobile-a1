local opening = true
local moving = true
local openingDistance = 70.0
local openSpeed = 35
local currentOpeningDistance = 0.0

function open()
    opening = true
    moving = true

    local pos = Vec3()

    Magic.AudioManager.PlaySound("",pos)
end

function close()
    opening = false
    moving = true
end

function toggle()
    opening = not opening
    moving = true
end


function update(entity)
    local prevOpenDistance = currentOpeningDistance;
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
        Magic.Log(Magic.LogLevel.info,currentOpeningDistance)
    end
    local transform = entity.transform
    local localPos = transform.localPosition

    localPos.z = localPos.z + (currentOpeningDistance - prevOpenDistance)
    
    transform.localPosition = localPos
end