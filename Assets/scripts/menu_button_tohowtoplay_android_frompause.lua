function OnButtonClicked(entity)
    if (Magic.IsAndroid()) then
        Magic.LoadSceneAdditive("scenes/how to play android.scene");
        Magic.UnloadScene("pausemenu");
    end
end