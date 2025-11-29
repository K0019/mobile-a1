local thisEntity;

function start(entity)
    thisEntity = entity
end

function OnCollisionEnter(entity)
    local physicsComp = thisEntity:GetPhysicsComp()
    if physicsComp:Exists() then
        if not physicsComp.isKinematic then
            local layerComp = entity:GetEntityLayerComponent()
            if layerComp:Exists() then
                if layerComp.layer == Magic.EntityLayer.Enemy then
                    local healthComp = entity:GetHealthComponent()
                    if healthComp:Exists() then
                        local vel = physicsComp.linearVelocity
                        local dir = Magic.Vec3(vel.x, 0.0, vel.z)
                        healthComp:TakeDamage(10.0, dir)
                        thisEntity:Destroy()
                    end
                elseif not (layerComp.layer == Magic.EntityLayer.Player) then
                    physicsComp.isKinematic = true
                    physicsComp.linearVelocity = Magic.Vec3(0.0, 0.0, 0.0)
                    physicsComp.angularVelocity = Magic.Vec3(0.0, 0.0, 0.0)
                end
            end
        end
    end
end