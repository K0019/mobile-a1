#include "Game/MaterialSwapper.h"
#include "Editor/Containers/GUICollection.h"
#include "Graphics/RenderComponent.h"

MaterialSwapperComponent::MaterialSwapperComponent()
    : inited{ false }
{
}

void MaterialSwapperComponent::ToggleMaterialSwap(bool swapped)
{
    ecs::EntityHandle thisEntity{ ecs::GetEntity(this) };
    if (ecs::CompHandle<RenderComponent> renderComp{ thisEntity->GetComp<RenderComponent>() })
    {
        if (!inited)
        {
            for (auto material : renderComp->GetMaterialsList())
                defaultMaterials.push_back(material);
            inited = true;
        }

        int matIndex = 0;
        for (auto& material : renderComp->GetMaterialsList())
        {
            material = swapped ? swapMaterial : defaultMaterials[matIndex];
            ++matIndex;
        }
    }
    else
    {
        CONSOLE_LOG(LogLevel::LEVEL_ERROR) << "No render comp attached!!!";
    }
}


void MaterialSwapperComponent::EditorDraw()
{
    const std::string* materialText{ ST<AssetManager>::Get()->Editor_GetName(swapMaterial.GetHash()) };
    gui::TextUnformatted("Material");
    gui::TextBoxReadOnly("##", materialText ? materialText->c_str() : "");
    gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
        swapMaterial = hash;
        });

}