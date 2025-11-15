local opening = true
local openingDistance = 70.0
local openSpeed = 3.50
local currentOpeningDistance = 0.0

function open()
    opening = true
end

function update(entity)
	if(opening) then
        local prevOpenDistance = currentOpeningDistance;
        currentOpeningDistance = currentOpeningDistance + Magic.DeltaTime() * openSpeed
        if(currentOpeningDistance > openingDistance) then
            currentOpeningDistance = openingDistance
            opening = false
        end
        local transform = entity.transform
        local localPos = transform.localPosition

        
        -- Magic.Log(Magic.LogLevel.info,1.0/Magic.DeltaTime())

        -- This fails
        localPos.z = localPos.z + (currentOpeningDistance - prevOpenDistance)
        
        transform.localPosition = localPos
    end
end