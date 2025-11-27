local thisEntity
function start(entity)
    thisEntity = entity
end
function OnHealthDepleted()
    Magic.AudioManager.PlaySound3D("mc death "..(Magic.Random.RangeInt(0,5)+1),false,thisEntity.transform.worldPosition)
end

function OnDamaged(amount,direction)
    Magic.AudioManager.PlaySound3D("mc hurt "..(Magic.Random.RangeInt(0,5)+1),false,thisEntity.transform.worldPosition)
end