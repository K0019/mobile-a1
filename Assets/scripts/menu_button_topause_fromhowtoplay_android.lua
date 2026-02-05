function OnButtonClicked(entity)
    if (Magic.IsAndroid()) then
        return
    end
    Magic.LoadSceneAdditive("scenes/pausemenu.scene");
    Magic.UnloadScene("how to play android");
end