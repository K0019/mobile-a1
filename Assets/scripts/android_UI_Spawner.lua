
function start(entity)
    -- We only need the UI if on Android
    if Magic.GetIsAndroid() then
        Magic.LoadSceneAdditive("Scenes/buttonandroidscene.scene")
    end

end

