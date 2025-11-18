local moveSpeed = 2.0
local rotationSpeed = 1440
local stunTimePerHit = 0.2
local groundFriction = 0.5
local dodgeCooldown = 1.0
local dodgeDuration = 0.2
local dodgeSpeed = 6.0

local movementVector = Vec2()

local physicsComp
local heldItem

function start(entity)
physicsComp = entity.GetPhysicsComp()
end

function update(entity)
	if heldItem then
        heldItem.transform.
    end
end