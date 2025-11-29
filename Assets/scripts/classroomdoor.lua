local opening = true
local moving = true
local openingDistance = 0.6
local openSpeed = 2.4
local currentOpeningDistance = 0.0
local openingDoorEntity
local closingDoorEntity
local thisEntity

local isClosed = false
local isOpen = false

function open()
    opening = true
    moving = true
    if not isOpen then
        Magic.AudioManager.PlaySound3DWithVolume("door open",false,thisEntity.transform.worldPosition,1)
    end

end

function close()
    opening = false
    moving = true
    if not isClosed then
        Magic.AudioManager.PlaySound3DWithVolume("door open",false,thisEntity.transform.worldPosition,1)
    end
end

function toggle()
    opening = not opening
    moving = true
    Magic.AudioManager.PlaySound3DWithVolume("door open",false,thisEntity.transform.worldPosition,1)
end

function start(entity)
    thisEntity = entity
    -- We decide whether the door starts open if the name has "Open"
    local nameComp = entity:GetNameComponent()
    Magic.Log(Magic.LogLevel.info, nameComp.name)
    if(nameComp.name == "Door_Classroom_Open") then
        opening = true
    else
        opening = false
    end

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
        isOpen = false
        isClosed=false
        if(opening) then
            currentOpeningDistance = currentOpeningDistance + Magic.DeltaTime() * openSpeed

            if(currentOpeningDistance > openingDistance) then
                currentOpeningDistance = openingDistance
                moving = false
                isOpen = true
            end
        else
            currentOpeningDistance = currentOpeningDistance - Magic.DeltaTime() * openSpeed

            if(currentOpeningDistance < 0) then
                currentOpeningDistance = 0
                moving = false
                isClosed=true
            end
        end
    end
    localPos.z = localPos.z + (currentOpeningDistance - prevOpenDistance)
    transform.localPosition = localPos
end