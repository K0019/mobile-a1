-- DigiPen Splash Screen Script
-- Fades in, holds, fades out, then loads game scene

local STATE_FADE_IN = 1
local STATE_HOLD = 2
local STATE_FADE_OUT = 3

local state = STATE_FADE_IN
local timer = 0.0
local fadeInTime = 1.5
local holdTime = 2.0
local fadeOutTime = 1.5
local sprite

function start(entity)
    sprite = entity:GetSpriteComponent()
    if sprite:Exists() then
        -- Start fully transparent
        sprite.color = Magic.Vec4(1.0, 1.0, 1.0, 0.0)
    end
end

function update(entity)
    local dt = Magic.DeltaTime()
    timer = timer + dt

    if state == STATE_FADE_IN then
        local alpha = math.min(timer / fadeInTime, 1.0)
        sprite.color = Magic.Vec4(1.0, 1.0, 1.0, alpha)
        if timer >= fadeInTime then
            timer = 0.0
            state = STATE_HOLD
        end

    elseif state == STATE_HOLD then
        -- Optional: check for any key press to skip
        -- if Magic.GetButtonDown("Jump") then
        --     timer = 0.0
        --     state = STATE_FADE_OUT
        -- end

        if timer >= holdTime then
            timer = 0.0
            state = STATE_FADE_OUT
        end

    elseif state == STATE_FADE_OUT then
        local alpha = 1.0 - math.min(timer / fadeOutTime, 1.0)
        sprite.color = Magic.Vec4(1.0, 1.0, 1.0, alpha)
        if timer >= fadeOutTime then
            Magic.LoadScene("scenes/DefaultScene.scene")
        end
    end
end
