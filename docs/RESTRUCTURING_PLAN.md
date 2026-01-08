# Project Restructuring Plan: Unified MagicEngine

## Overview

Restructure from fragmented multi-library architecture to a clean unified structure.

### Current State (main branch)
```
Build Targets: 5 libraries + 2 executables
├── ryEngine-Core        (109 files) - Vulkan, platform, utilities
├── ryEngine-ImGui       (21 files)  - ImGui integration
├── ryEngine-Tools       (included above)
├── MagicEngine-Core     (391 files) - ECS, components, editor, game logic
├── EngineScripting      (C#)        - DEPRECATED, remove
├── AssetCompiler        (exe)
└── Editor               (exe)
```

### Target State
```
Build Targets: 1 library + 2 executables (+ future Game exe)
├── MagicEngine          (static lib) - ALL engine code unified
├── AssetCompiler        (exe)
├── Editor               (exe)        - Editor-specific UI code
└── Game                 (exe)        - FUTURE: ships without editor
```

---

## Phase 1: Git Setup

### Step 1.1: Create new branch from main
```bash
git checkout main
git pull origin main
git checkout -b unified-engine
```

### Step 1.2: Verify clean state
```bash
git status  # Should be clean
```

---

## Phase 2: Directory Structure Creation

### Step 2.1: Create new MagicEngine/ directory structure
```
MagicEngine/
├── CMakeLists.txt
├── pch.h
│
├── core/                    # From ryEngine-core/core/
│   ├── engine/
│   └── platform/
│       ├── android/
│       └── desktop/
│
├── ecs/                     # From MagicEngine-core/ECS/
│   └── (14 files)
│
├── components/              # From MagicEngine-core/Components/
│
├── engine/                  # From MagicEngine-core/Engine/
│   ├── behavior_tree/
│   ├── events/
│   ├── resources/
│   └── platform/
│
├── graphics/                # MERGED: ryEngine + hina-vk + new code
│   ├── backend/             # hina-vk sources (built inline)
│   │   ├── hina_vk.c
│   │   ├── hina_vk.h
│   │   ├── hina_internal.h
│   │   ├── hina_vk_impl.cpp
│   │   ├── spirv_reflect.c
│   │   └── spirv_reflect.h
│   ├── vulkan/              # From ryEngine-core/graphics/vulkan/ (legacy, phase out later)
│   ├── features/            # From ryEngine-core/graphics/features/
│   ├── ui/                  # From ryEngine-core/graphics/ui/
│   ├── gfx_renderer.cpp     # NEW: from graphics_refactor branch
│   ├── gfx_renderer.h
│   ├── hina_context.cpp
│   ├── hina_context.h
│   └── ...
│
├── audio/                   # From MagicEngine-core/Engine/Audio (if exists)
│
├── physics/                 # From MagicEngine-core/Physics/
│
├── navigation/              # Recast/Detour integration
│
├── scripting/               # From MagicEngine-core/Scripting/ (Lua only)
│   └── lua_library/
│
├── ui/                      # From MagicEngine-core/UI/
│
├── 3dui/                    # From MagicEngine-core/3DUI/
│
├── tween/                   # From MagicEngine-core/Tween/
│
├── managers/                # From MagicEngine-core/Managers/
│
├── game/                    # From MagicEngine-core/Game/
│
├── math/                    # From ryEngine-core/math/
│
├── logging/                 # From ryEngine-core/logging/
│
├── resource/                # From ryEngine-core/resource/
│
├── utilities/               # Merged from both
│
├── vfs/                     # From ryEngine-core/VFS/
│
└── imgui/                   # From ryEngine-modules/imgui/
    ├── base/
    └── features/
```

### Step 2.2: Create editor/ structure for editor-specific code
```
editor/
├── CMakeLists.txt
├── main_desktop.cpp
├── application.cpp
├── application.h
├── pch.h
│
├── panels/                  # FROM MagicEngine-core/Editor/
│   ├── hierarchy.cpp
│   ├── hierarchy.h
│   ├── inspector.cpp
│   ├── inspector.h
│   ├── asset_browser.cpp
│   ├── ... (all 70 editor files)
│
├── gui/                     # FROM MagicEngine-core/Editor/Containers/
│   ├── gui_collection.cpp
│   ├── gui_collection.h
│   ├── gui_as_ecs.cpp
│   └── gui_as_ecs.h
│
└── imgui/                   # Editor-specific ImGui setup
    └── imgui_context.cpp
```

---

## Phase 3: File Migration

### Step 3.1: Migrate ryEngine-core (109 files)

| From | To |
|------|----|
| `ryEngine-core/core/` | `MagicEngine/core/` |
| `ryEngine-core/graphics/` | `MagicEngine/graphics/` |
| `ryEngine-core/logging/` | `MagicEngine/logging/` |
| `ryEngine-core/math/` | `MagicEngine/math/` |
| `ryEngine-core/resource/` | `MagicEngine/resource/` |
| `ryEngine-core/utilities/` | `MagicEngine/utilities/` |
| `ryEngine-core/VFS/` | `MagicEngine/vfs/` |

### Step 3.2: Migrate MagicEngine-core engine code (321 files, excluding Editor)

| From | To |
|------|----|
| `MagicEngine-core/ECS/` | `MagicEngine/ecs/` |
| `MagicEngine-core/Components/` | `MagicEngine/components/` |
| `MagicEngine-core/Engine/` | `MagicEngine/engine/` |
| `MagicEngine-core/Physics/` | `MagicEngine/physics/` |
| `MagicEngine-core/Scripting/` | `MagicEngine/scripting/` |
| `MagicEngine-core/UI/` | `MagicEngine/ui/` |
| `MagicEngine-core/3DUI/` | `MagicEngine/3dui/` |
| `MagicEngine-core/Tween/` | `MagicEngine/tween/` |
| `MagicEngine-core/Managers/` | `MagicEngine/managers/` |
| `MagicEngine-core/Game/` | `MagicEngine/game/` |
| `MagicEngine-core/Graphics/` | `MagicEngine/graphics/legacy/` |
| `MagicEngine-core/Utilities/` | Merge into `MagicEngine/utilities/` |

### Step 3.3: Migrate Editor code to editor/ (70 files)

| From | To |
|------|----|
| `MagicEngine-core/Editor/*.cpp/h` | `editor/panels/` |
| `MagicEngine-core/Editor/Containers/` | `editor/gui/` |

### Step 3.4: Migrate ryEngine-modules (21 files)

| From | To |
|------|----|
| `ryEngine-modules/imgui/` | `MagicEngine/imgui/` |
| `ryEngine-modules/tools/` | `MagicEngine/tools/` or `AssetCompiler/` |

### Step 3.5: Add hina-vk sources (from graphics_refactor)

Copy from `graphics_refactor:extern/hina-vk/` to `MagicEngine/graphics/backend/`:
- `hina_vk.c`
- `hina_vk.h` (from include/)
- `hina_internal.h`
- `hina_vk_impl.cpp`
- `spirv_reflect.c`
- `spirv_reflect.h`
- `spirv.h`

---

## Phase 4: CMake Restructuring

### Step 4.1: Root CMakeLists.txt changes

```cmake
# REMOVE these:
# add_subdirectory(ryEngine-core)
# add_subdirectory(ryEngine-modules/imgui)
# add_subdirectory(ryEngine-modules/tools)
# add_subdirectory(MagicEngine-core)
# add_subdirectory(EngineScripting)

# KEEP/ADD these:
add_subdirectory(MagicEngine)        # Single unified library
add_subdirectory(AssetCompiler)
add_subdirectory(editor)
# add_subdirectory(game)             # FUTURE
```

### Step 4.2: MagicEngine/CMakeLists.txt (new unified lib)

```cmake
# MagicEngine/CMakeLists.txt
# Single unified engine library

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "*.cpp" "*.c" "*.h" "*.hpp" "*.ipp"
)

# Exclude editor-specific code if any leaked in
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/.*")

add_library(MagicEngine STATIC ${ENGINE_SOURCES})

target_include_directories(MagicEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/backend  # hina-vk headers
)

target_link_libraries(MagicEngine PUBLIC
    # vcpkg dependencies
    glm::glm-header-only
    fmt::fmt
    quill::quill
    Boost::boost
    GPUOpen::VulkanMemoryAllocator
    volk::volk_headers
    Vulkan::Headers
    # ... other deps
)

target_link_libraries(MagicEngine PRIVATE
    # Private build dependencies
    glslang::glslang
    spirv-cross-core
    # ...
)

target_compile_definitions(MagicEngine PUBLIC
    VK_NO_PROTOTYPES=1
    $<$<CONFIG:Debug>:IMGUI_ENABLED>
)
```

### Step 4.3: editor/CMakeLists.txt

```cmake
# editor/CMakeLists.txt

file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS
    "*.cpp" "*.h"
)

add_executable(Editor WIN32 ${EDITOR_SOURCES})

target_link_libraries(Editor PRIVATE
    MagicEngine  # Single dependency!
)

target_include_directories(Editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
```

---

## Phase 5: Include Path Updates

### Step 5.1: Create compatibility headers (optional, for gradual migration)

For smooth transition, create forwarding headers:
```cpp
// MagicEngine/compat/ecs.h
#pragma once
#include "MagicEngine/ecs/ECS.h"
```

### Step 5.2: Bulk search-replace include paths

| Old Include | New Include |
|-------------|-------------|
| `#include "ECS/ECS.h"` | `#include "ecs/ECS.h"` |
| `#include "ryEngine-core/..."` | `#include "..."` |
| `#include "Engine/..."` | `#include "engine/..."` |
| `#include "Editor/..."` | `#include "panels/..."` (in editor/) |

---

## Phase 6: Cleanup

### Step 6.1: Remove old directories
```bash
git rm -r ryEngine-core/
git rm -r ryEngine-modules/
git rm -r MagicEngine-core/
git rm -r EngineScripting/
```

### Step 6.2: Remove EnTT from vcpkg.json (if present)

### Step 6.3: Update .gitignore if needed

---

## Phase 7: Verification

### Step 7.1: Build test
```bash
cmake -B build -S .
cmake --build build --target MagicEngine
cmake --build build --target Editor
cmake --build build --target AssetCompiler
```

### Step 7.2: Run test
```bash
./build/bin/Debug/Editor.exe
```

### Step 7.3: Verify functionality
- [ ] Editor launches
- [ ] Scene loads
- [ ] Hierarchy panel works
- [ ] Inspector panel works
- [ ] Asset browser works
- [ ] Viewport renders

---

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| Circular dependencies | Medium | Careful ordering in CMake |
| Include path breakage | High | Batch search-replace, compatibility headers |
| ImGui context issues | Low | Already working, just moving |
| Build time increase | Low | Single lib compiles in parallel |

---

## Rollback Plan

If things go wrong:
```bash
git checkout main
git branch -D unified-engine
```

---

## Timeline Estimate

| Phase | Estimated Effort |
|-------|-----------------|
| Phase 1: Git Setup | 5 min |
| Phase 2: Directory Structure | 15 min |
| Phase 3: File Migration | 30-60 min |
| Phase 4: CMake Restructuring | 30-60 min |
| Phase 5: Include Path Updates | 60-120 min |
| Phase 6: Cleanup | 15 min |
| Phase 7: Verification | 30-60 min |

**Total: 3-6 hours** (depending on include path complexity)

---

## Future Work (After Restructuring)

1. **Add hina-vk rendering path** - Replace ryEngine vulkan with hina-vk
2. **Port gfx_renderer** - Bring over from graphics_refactor
3. **Create Game target** - Exe without editor code
4. **Remove legacy vulkan code** - Once hina-vk is fully integrated
