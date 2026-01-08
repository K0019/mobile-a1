#include "application.h"
#include <core/engine/engine.h>
#include "Engine/Engine.h"

// Editor panel includes
#ifdef IMGUI_ENABLED
#include "Editor/Editor.h"
#include "Editor/MainMenuBar.h"
#include "Editor/Popup.h"
#include "Editor/Console.h"
#include "Editor/Performance.h"
#include "Editor/Inspector.h"
#include "Editor/AssetBrowser.h"
#include "Editor/Hierarchy.h"
#include "Editor/Containers/GUIAsECS.h"
#include "Graphics/CustomViewport.h"
#include "Utilities/Serializer.h"
#include <fstream>
#include <sstream>

namespace
{
    void saveEditorState(const char* filename) {
        ecs::SwitchToPool(ecs::POOL::EDITOR_GUI);
        Serializer serializer{ filename };
        serializer.Serialize("show_console", ecs::GetCompsBegin<editor::Console>() != ecs::GetCompsEnd<editor::Console>());
        serializer.Serialize("show_performance", ecs::GetCompsBegin<editor::PerformanceWindow>() != ecs::GetCompsEnd<editor::PerformanceWindow>());
        serializer.Serialize("show_editor", ecs::GetCompsBegin<editor::Inspector>() != ecs::GetCompsEnd<editor::Inspector>());
        serializer.Serialize("show_browser", ecs::GetCompsBegin<editor::AssetBrowser>() != ecs::GetCompsEnd<editor::AssetBrowser>());
        serializer.Serialize("show_hierarchy", ecs::GetCompsBegin<editor::Hierarchy>() != ecs::GetCompsEnd<editor::Hierarchy>());
        ecs::SwitchToPool(ecs::POOL::DEFAULT);
    }

    void loadEditorState(const char* filename) {
        std::ifstream t(filename);
        std::stringstream buffer;
        buffer << t.rdbuf();
        Deserializer deserializer{ buffer.str() };
        if (!deserializer.IsValid())
            return;

        auto LoadWindowOpen{ [&deserializer, b = false]<typename WindowType>(const char* varName) mutable -> void {
            deserializer.DeserializeVar(varName, &b);
            if (b)
                editor::CreateGuiWindow<WindowType>();
        } };

        LoadWindowOpen.template operator()<editor::Console>("show_console");
        LoadWindowOpen.template operator()<editor::PerformanceWindow>("show_performance");
        LoadWindowOpen.template operator()<editor::Inspector>("show_editor");
        LoadWindowOpen.template operator()<editor::AssetBrowser>("show_browser");
        LoadWindowOpen.template operator()<editor::Hierarchy>("show_hierarchy");
    }
}
#endif

void Application::Initialize(Context& context)
{
    magicEngine.Init(context);

    // Register editor-specific systems (must be done here since editor headers
    // are not available when MagicEngine is compiled)
#ifdef IMGUI_ENABLED
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::EditorSystem{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::MainMenuBar{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_EDITOR, editor::Popup{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_INPUT, editor::EditorInputSystem{});
    ecs::AddSystem(ECS_LAYER::PERMANENT_RENDER, editor::SelectedEntityBorderDrawSystem{});

    // Load saved editor window state
    loadEditorState("imgui.json");

    // Create default viewport
    auto worldExtents{ IntVec2{ 1920, 1080 } };
    editor::CreateGuiWindow<CustomViewport>(static_cast<unsigned int>(worldExtents.x), static_cast<unsigned int>(worldExtents.y));
#endif
}

void Application::Update([[maybe_unused]] Context& context, FrameData& frame)
{
    magicEngine.ExecuteFrame(frame);
}

void Application::Shutdown([[maybe_unused]] Context& context)
{
#ifdef IMGUI_ENABLED
    saveEditorState("imgui.json");
#endif
    magicEngine.shutdown();
}
