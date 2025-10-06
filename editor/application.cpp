#include "application.h"

#include <imgui.h>
#include <graphics/features/grid_feature.h>
#include <graphics/features/scene_feature.h>
#include "math/utils_math.h"

void Application::Initialize(Context& context)
{
  gridFeatureHandle_ = context.renderer->CreateFeature<GridFeature>();
  sceneFeatureHandle_ = context.renderer->CreateFeature<SceneRenderFeature>();
  imguiContext_ = std::make_unique<editor::ImGuiContext>(context);
  imguiContext_->setFont("Assets/fonts/TX-02-Regular.ttf", 13.0f);
  sceneLoader_ = std::make_unique<Resource::SceneLoader>(*context.resourceMngr);
  //const std::filesystem::path testScenePath = "assets/AlphaBlendModeTest.glb";
   const std::filesystem::path testScenePath = "Assets/DamagedHelmet.glb";
  //const std::filesystem::path testScenePath = "assets/sponza-gltf-pbr/sponza.glb";

  /*AssetLoading::ProcessedTexture irradiance;
  AssetLoading::TextureLoading::loadFromFile("assets/piazza_bologni_1k_irradiance.ktx",irradiance, false, vk::TextureType::TexCube);
  irradianceMap = context.assetSystem->getTextureBindlessIndex(context.assetSystem->createTexture(irradiance));

  AssetLoading::ProcessedTexture prefilter;
  AssetLoading::TextureLoading::loadFromFile("assets/piazza_bologni_1k_prefilter.ktx",prefilter, false, vk::TextureType::TexCube);
  prefilteredEnvMap = context.assetSystem->getTextureBindlessIndex(context.assetSystem->createTexture(prefilter));

  AssetLoading::ProcessedTexture brdf;
  AssetLoading::TextureLoading::loadFromFile("assets/piazza_bologni_1k_charlie.ktx",brdf, false, vk::TextureType::Tex2D);
  brdfLUT = context.assetSystem->getTextureBindlessIndex(context.assetSystem->createTexture(brdf));*/
  if(exists(testScenePath))
  {
    auto loadResult = sceneLoader_->loadScene(testScenePath);
    if(loadResult.success)
    {
      loadedScene_ = std::move(loadResult.scene);
      LOG_INFO("Successfully loaded scene '{}' with {} objects ({} renderable)", loadedScene_.name, loadedScene_.objects.size(), std::count_if(loadedScene_.objects.begin(), loadedScene_.objects.end(), [](const auto& obj) { return obj.isRenderable(); }));

      LOG_INFO("Loading stats: {:.1f}ms, {} meshes, {} materials, {} textures", loadResult.stats.totalTimeMs, loadResult.stats.meshesLoaded, loadResult.stats.materialsLoaded, loadResult.stats.texturesLoaded);
    }
    else
    {
      LOG_WARNING("Failed to load scene from {}: {}", testScenePath.string(), loadResult.warnings.empty() ? "Unknown error" : loadResult.warnings[0]);
    }
  }
}

void Application::Update(Context& context, FrameData& frame)
{
  imguiContext_->beginFrame();
  int width =  Core::Display().GetWidth();
  int height = Core::Display().GetHeight();
  positioner_.movement_.forward_ = Input::GetKey(Key::W);
  positioner_.movement_.backward_ = Input::GetKey(Key::S);
  positioner_.movement_.left_ = Input::GetKey(Key::A);
  positioner_.movement_.right_ = Input::GetKey(Key::D);
  positioner_.movement_.up_ = Input::GetKey(Key::Num1);
  positioner_.movement_.down_ = Input::GetKey(Key::Num2);
  positioner_.movement_.fastSpeed_ = Input::GetKey(Key::LeftShift);
  if(Input::GetKey(Key::Space))
  {
    positioner_.lookAt(vec3(0.0f, 1.0f, -1.5f), vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
    positioner_.setSpeed(vec3(0.0f));
  }
  positioner_.update(frame.deltaTime, Input::GetMouseDelta(), Input::GetMouseButton(MouseButton::Right));
  if(Input::GetKey(Key::F11))
  {
    Core::Display().setFullscreen(!Core::Display().isFullscreen());
  }
  if(ImGui::CollapsingHeader("Tone Mapping Settings", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto settings = context.renderer->GetToneMappingSettings();
    bool changed = false;

    // Mode selection
    const char* toneMappingModes[] = { "None", "Reinhard", "Uchimura", "ACES", "Khronos PBR", "Uncharted 2", "Unreal" };
    int currentMode = settings.mode;
    if(ImGui::Combo("Tone Mapping Mode", &currentMode, toneMappingModes, IM_ARRAYSIZE(toneMappingModes)))
    {
      settings.mode = static_cast<ToneMappingSettings::Mode>(currentMode);
      changed = true;
    }

    // Global settings
    if(ImGui::SliderFloat("Exposure", &settings.exposure, 0.1f, 4.0f, "%.2f"))
      changed = true;

    // Mode-specific controls
    switch(settings.mode)
    {
      case ToneMappingSettings::Reinhard:
      {
        ImGui::Separator();
        ImGui::Text("Reinhard Parameters");
        if(ImGui::SliderFloat("Max White Level", &settings.maxWhite, 0.5f, 10.0f, "%.1f"))
          changed = true;
        ImGui::TextDisabled("Higher values = brighter highlights");
        break;
      }

      case ToneMappingSettings::Uchimura:
      {
        ImGui::Separator();
        ImGui::Text("Uchimura Parameters");
        if(ImGui::SliderFloat("Max Display Brightness (P)", &settings.P, 0.8f, 1.5f, "%.2f"))
          changed = true;
        if(ImGui::SliderFloat("Contrast (a)", &settings.a, 0.8f, 1.5f, "%.2f"))
          changed = true;
        if(ImGui::SliderFloat("Linear Section Start (m)", &settings.m, 0.18f, 0.30f, "%.3f"))
          changed = true;
        if(ImGui::SliderFloat("Linear Section Length (l)", &settings.l, 0.15f, 0.50f, "%.3f"))
          changed = true;
        if(ImGui::SliderFloat("Black Tightness (c)", &settings.c, 1.0f, 3.0f, "%.1f"))
          changed = true;
        if(ImGui::SliderFloat("Pedestal (b)", &settings.b, 0.0f, 0.04f, "%.3f"))
          changed = true;
        break;
      }

      case ToneMappingSettings::KhronosPBR:
      {
        ImGui::Separator();
        ImGui::Text("Khronos PBR Neutral");
        ImGui::TextDisabled("Fixed parameters for color accuracy");
        ImGui::Text("Start Compression: 0.76");
        ImGui::Text("Desaturation: 0.15");
        ImGui::TextDisabled("Designed for faithful PBR color reproduction");
        break;
      }

      case ToneMappingSettings::ACES:
      {
        ImGui::Separator();
        ImGui::Text("ACES Film Curve");
        ImGui::TextDisabled("Standard constants: A=2.51, B=0.03, C=2.43, D=0.59, E=0.14");
        ImGui::TextDisabled("Industry standard for film and VFX");
        break;
      }

      case ToneMappingSettings::Uncharted2:
      {
        ImGui::Separator();
        ImGui::Text("Uncharted 2 (Hable)");
        ImGui::TextDisabled("Gaming industry standard filmic curve");
        ImGui::Text("Shoulder Strength: 0.15");
        ImGui::Text("Linear Strength: 0.50");
        ImGui::Text("Linear Angle: 0.10");
        ImGui::Text("Toe Strength: 0.20");
        ImGui::TextDisabled("Popularized by Uncharted 2, widely used in games");
        break;
      }

      case ToneMappingSettings::Unreal:
      {
        ImGui::Separator();
        ImGui::Text("Unreal Engine");
        ImGui::TextDisabled("Fast filmic approximation used in UE4");
        ImGui::Text("Formula: x / (x + 0.155) * 1.019");
        ImGui::TextDisabled("Note: Has gamma correction baked in");
        break;
      }

      case ToneMappingSettings::None:
      {
        ImGui::Separator();
        ImGui::TextDisabled("No tone mapping applied - values clamped to [0,1]");
        break;
      }
    }

    // Updated preset buttons
    ImGui::Separator();
    ImGui::Text("Presets");

    if(ImGui::Button("Neutral"))
    {
      settings.mode = ToneMappingSettings::Uchimura;
      settings.exposure = 1.0f;
      settings.P = 1.0f;
      settings.a = 1.0f;
      settings.m = 0.22f;
      settings.l = 0.4f;
      settings.c = 1.33f;
      settings.b = 0.0f;
      changed = true;
    }
    ImGui::SameLine();

    if(ImGui::Button("Cinematic"))
    {
      settings.mode = ToneMappingSettings::ACES;
      settings.exposure = 1.2f;
      changed = true;
    }
    ImGui::SameLine();

    if(ImGui::Button("Gaming"))
    {
      settings.mode = ToneMappingSettings::Uncharted2;
      settings.exposure = 1.1f;
      changed = true;
    }
    ImGui::SameLine();

    if(ImGui::Button("Fast"))
    {
      settings.mode = ToneMappingSettings::Unreal;
      settings.exposure = 1.0f;
      changed = true;
    }

    // Second row of presets
    if(ImGui::Button("Bright"))
    {
      settings.mode = ToneMappingSettings::Reinhard;
      settings.exposure = 1.0f;
      settings.maxWhite = 4.0f;
      changed = true;
    }
    ImGui::SameLine();

    if(ImGui::Button("PBR-Accurate"))
    {
      settings.mode = ToneMappingSettings::KhronosPBR;
      settings.exposure = 1.0f;
      changed = true;
    }

    // Apply changes
    if(changed)
    {
      context.renderer->UpdateToneMappingSettings(settings);
    }

    // Debug info
    if(ImGui::TreeNode("Debug Info"))
    {
      ImGui::Text("Current Mode: %s", toneMappingModes[static_cast<int>(settings.mode)]);
      ImGui::Text("Exposure: %.3f", settings.exposure);

      if(settings.mode == ToneMappingSettings::Uchimura)
      {
        ImGui::Text("Uchimura - P:%.2f a:%.2f m:%.3f l:%.2f c:%.1f b:%.3f",
                    settings.P, settings.a, settings.m, settings.l, settings.c, settings.b);
      }
      else if(settings.mode == ToneMappingSettings::Reinhard)
      {
        ImGui::Text("Reinhard - Max White: %.1f", settings.maxWhite);
      }

      ImGui::TreePop();
    }
  }
  if(ImGui::Begin("Scene"))
  {
    const auto& registry = imguiContext_->GetTransientRegistry();

    if(auto sceneColorID = registry.QueryBindlessID("ImGuiSceneView"))
    {
      ImVec2 previewSize(256, 144);
      ImGui::Image(*sceneColorID, previewSize);
    }
    else
    {
      ImGui::Text("Scene color not available");
    }
    // Debug panel showing all available transients
    if(ImGui::CollapsingHeader("Available Transients"))
    {
      for(const std::string& resourceName : registry.GetAvailableResources())
      {
        if(auto bindlessID = registry.QueryBindlessID(resourceName))
        {
          ImGui::Text("%s: ID %u", resourceName.c_str(), *bindlessID);
        }
      }
    }
    ImGui::End();
  }

  if(ImGui::Begin("Lighting System"))
  {
    ImGui::Text("Scene: %s", loadedScene_.name.c_str());
    ImGui::Text("Total Lights: %zu", loadedScene_.lights.size());
    ImGui::Separator();

    // Add new light button
    if(ImGui::Button("Add Point Light"))
    {
      SceneLight newLight;
      newLight.type = LightType::Point;
      newLight.position = camera.getPosition() + positioner_.getForward() * 2.0f; // Place in front of camera
      newLight.color = vec3(1.0f, 1.0f, 1.0f);
      newLight.intensity = 1.0f;
      newLight.name = "Light_" + std::to_string(loadedScene_.lights.size());
      loadedScene_.lights.push_back(newLight);
    }
    ImGui::SameLine();
    if(ImGui::Button("Add Directional Light"))
    {
      SceneLight newLight;
      newLight.type = LightType::Directional;
      newLight.direction = vec3(0.0f, -1.0f, -0.5f);
      newLight.color = vec3(1.0f, 0.9f, 0.8f);
      newLight.intensity = 2.0f;
      newLight.name = "DirLight_" + std::to_string(loadedScene_.lights.size());
      loadedScene_.lights.push_back(newLight);
    }

    ImGui::Separator();

    // Light list and editing
    for(size_t i = 0; i < loadedScene_.lights.size(); ++i)
    {
      SceneLight& light = loadedScene_.lights[i];

      ImGui::PushID(static_cast<int>(i));

      if(ImGui::CollapsingHeader(light.name.c_str()))
      {
        // Light type
        const char* lightTypes[] = { "Directional", "Point", "Spot", "Area" };
        int currentType = static_cast<int>(light.type);
        if(ImGui::Combo("Type", &currentType, lightTypes, 4))
          light.type = static_cast<LightType>(currentType);

        // Position (for non-directional lights)
        if(light.type != LightType::Directional)
        {
          ImGui::DragFloat3("Position", &light.position.x, 0.1f);
        }

        // Direction (for directional and spot lights)
        if(light.type == LightType::Directional || light.type == LightType::Spot)
        {
          ImGui::DragFloat3("Direction", &light.direction.x, 0.01f);
          light.direction = normalize(light.direction); // Keep normalized
        }

        // Color and intensity
        ImGui::ColorEdit3("Color", &light.color.x);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);

        // Attenuation (for point and spot lights)
        if(light.type == LightType::Point || light.type == LightType::Spot)
        {
          ImGui::DragFloat3("Attenuation (C,L,Q)", &light.attenuation.x, 0.001f, 0.0f, 10.0f);
        }

        // Spot light angles
        if(light.type == LightType::Spot)
        {
          float innerDeg = glm::degrees(light.innerConeAngle);
          float outerDeg = glm::degrees(light.outerConeAngle);
          if(ImGui::DragFloat("Inner Angle (deg)", &innerDeg, 1.0f, 0.0f, 90.0f))
            light.innerConeAngle = glm::radians(innerDeg);
          if(ImGui::DragFloat("Outer Angle (deg)", &outerDeg, 1.0f, 0.0f, 90.0f))
            light.outerConeAngle = glm::radians(outerDeg);
        }

        // Quick positioning buttons
        if(light.type != LightType::Directional)
        {
          if(ImGui::Button("Move to Camera"))
            light.position = camera.getPosition();
          ImGui::SameLine();
          if(ImGui::Button("Move Forward"))
            light.position = camera.getPosition() + positioner_.getForward() * 3.0f;
        }

        // Delete button
        if(ImGui::Button("Delete Light"))
        {
          loadedScene_.lights.erase(loadedScene_.lights.begin() + i);
          ImGui::PopID();
          break; // Exit loop since we modified the vector
        }
      }

      ImGui::PopID();
    }

    ImGui::Separator();

    // Quick presets
    if(ImGui::CollapsingHeader("Quick Presets"))
    {
      if(ImGui::Button("Clear All Lights"))
        loadedScene_.lights.clear();

      if(ImGui::Button("Basic 3-Point Setup"))
      {
        loadedScene_.lights.clear();

        // Key light
        SceneLight keyLight;
        keyLight.type = LightType::Point;
        keyLight.position = vec3(2.0f, 3.0f, 2.0f);
        keyLight.color = vec3(1.0f, 0.95f, 0.8f);
        keyLight.intensity = 5.0f;
        keyLight.name = "Key_Light";
        loadedScene_.lights.push_back(keyLight);

        // Fill light
        SceneLight fillLight;
        fillLight.type = LightType::Point;
        fillLight.position = vec3(-2.0f, 1.0f, 1.0f);
        fillLight.color = vec3(0.7f, 0.8f, 1.0f);
        fillLight.intensity = 2.0f;
        fillLight.name = "Fill_Light";
        loadedScene_.lights.push_back(fillLight);

        // Rim light
        SceneLight rimLight;
        rimLight.type = LightType::Point;
        rimLight.position = vec3(0.0f, 2.0f, -3.0f);
        rimLight.color = vec3(1.0f, 1.0f, 1.0f);
        rimLight.intensity = 3.0f;
        rimLight.name = "Rim_Light";
        loadedScene_.lights.push_back(rimLight);
      }
    }
  }
  ImGui::End();
  // **OBJECT TRANSFORM UI** (bonus)
  if(ImGui::Begin("Scene Objects"))
  {
    ImGui::Text("Total Objects: %zu", loadedScene_.objects.size());
    ImGui::Separator();

    for(size_t i = 0; i < loadedScene_.objects.size(); ++i)
    {
      SceneObject& obj = loadedScene_.objects[i];

      ImGui::PushID(static_cast<int>(i));

      if(ImGui::CollapsingHeader(obj.name.c_str()))
      {
        // Transform controls
        vec3 translation, rotation [[maybe_unused]], scale;

        // Extract transform components (basic decomposition)
        translation = vec3(obj.transform[3]);
        scale = vec3(length(vec3(obj.transform[0])),
                     length(vec3(obj.transform[1])),
                     length(vec3(obj.transform[2])));

        if(ImGui::DragFloat3("Position", &translation.x, 0.1f))
        {
          obj.transform[3] = vec4(translation, 1.0f);
        }

        if(ImGui::DragFloat3("Scale", &scale.x, 0.01f))
        {
          // Reapply scale (simplified)
          obj.transform[0] = vec4(normalize(vec3(obj.transform[0])) * scale.x, 0.0f);
          obj.transform[1] = vec4(normalize(vec3(obj.transform[1])) * scale.y, 0.0f);
          obj.transform[2] = vec4(normalize(vec3(obj.transform[2])) * scale.z, 0.0f);
        }

        // Object info
        ImGui::Text("Type: %s", obj.type == SceneObjectType::Mesh ? "Mesh" :
                    obj.type == SceneObjectType::Light ? "Light" :
                    obj.type == SceneObjectType::Camera ? "Camera" : "Empty");
        ImGui::Text("Renderable: %s", obj.isRenderable() ? "Yes" : "No");
      }

      ImGui::PopID();
    }
  }
  ImGui::End();

  frame.cameraPos = camera.getPosition();
  frame.viewMatrix = camera.getViewMatrix();
  frame.projMatrix = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);
  SceneRenderFeature::UpdateScene(sceneFeatureHandle_, loadedScene_, *context.resourceMngr, *context.renderer);
  if (
      auto params = static_cast<SceneRenderParams*>(context.renderer->
          GetFeatureParameterBlockPtr(sceneFeatureHandle_)))
  {
      params->irradianceTexture = irradianceMap;
      params->prefilterTexture = prefilteredEnvMap;
      params->brdfLUT = brdfLUT;
      params->environmentIntensity = 1.0f;
  }
  imguiContext_->endFrame();
}

void Application::Shutdown(Context& context)
{
  context.renderer->DestroyFeature(gridFeatureHandle_);
}
