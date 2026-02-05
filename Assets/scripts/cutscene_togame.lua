function update(entity)
    local videoComp = entity:GetVideoPlayerComponent()
    if ((videoComp:Exists() and videoComp:HasFinished()) or Magic.GetButtonDown("Light Attack")) then
        Magic.TransitionScene("scenes/DefaultScene.scene")
    end
end