function update(entity)
    local videoComp = entity:GetVideoPlayerComponent()
    if ((videoComp:Exists() and videoComp:HasFinished())) then
        Magic.TransitionScene("scenes/DefaultScene.scene")
    end
end
function OnButtonClicked()
    Magic.Log(Magic.LogLevel.info, "Button pressed, skipping cutscene.")
    Magic.TransitionScene("scenes/DefaultScene.scene")
end