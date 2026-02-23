-- Simple debug script that logs every update frame

function start(entity)
    Magic.Log(Magic.LogLevel.info, "[DEBUG] Script started")
end

function update(entity, dt)
    Magic.Log(Magic.LogLevel.info, "[DEBUG] Update called, dt = " .. tostring(dt))
end

function onDestroy(entity)
    Magic.Log(Magic.LogLevel.info, "[DEBUG] Script destroyed")
end