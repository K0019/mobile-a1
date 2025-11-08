local var = 1;

function update(entity)
    local nameComp = entity:GetNameComponent();
    if nameComp:Exists() then
        print(nameComp.name)
    end
    --Magic.TestFunction(entity);
    var = var + 1;
end