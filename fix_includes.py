#!/usr/bin/env python3
import os
import re

# Replacements to make
replacements = [
    ('Editor/IHiddenComponent.h', 'ECS/IHiddenComponent.h'),
    ('Editor/IEditorComponent.h', 'ECS/IEditorComponent.h'),
    ('graphics/interface.h', 'renderer/interface.h'),
    ('graphics/scene.h', 'renderer/scene.h'),
    ('graphics/gpu_data.h', 'renderer/gpu_data.h'),
    ('graphics/render_graph.h', 'renderer/render_graph.h'),
    ('graphics/linear_color.h', 'renderer/linear_color.h'),
    ('graphics/frame_data.h', 'renderer/frame_data.h'),
    ('graphics/render_feature.h', 'renderer/render_feature.h'),
    ('graphics/pool.h', 'renderer/pool.h'),
    ('graphics/shader.h', 'renderer/shader.h'),
    ('graphics/i_graph_observer.h', 'renderer/i_graph_observer.h'),
    ('graphics/i_renderer.h', 'renderer/i_renderer.h'),
    ('graphics/renderer.h', 'renderer/renderer.h'),
    ('graphics/OIT.h', 'renderer/OIT.h'),
    ('graphics/vulkan/vk_classes.h', 'renderer/vulkan/vk_classes.h'),
    ('graphics/vulkan/vk_util.h', 'renderer/vulkan/vk_util.h'),
    ('graphics/vulkan/vk_context.h', 'renderer/vulkan/vk_context.h'),
    ('graphics/vulkan/vk_allocator.h', 'renderer/vulkan/vk_allocator.h'),
    ('graphics/vulkan/vk_pipeline.h', 'renderer/vulkan/vk_pipeline.h'),
    ('graphics/features/', 'renderer/features/'),
    ('graphics/ui/', 'renderer/ui/'),
    ('graphics/animation_update.h', 'renderer/animation_update.h'),
    ('graphics/i_graph_observer.h', 'renderer/i_graph_observer.h'),
    ('utilities/util.h', 'core_utils/util.h'),
    ('utilities/clock.h', 'core_utils/clock.h'),
]

def process_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()

        modified = False
        for old, new in replacements:
            old_pattern = f'#include "{old}"'
            new_pattern = f'#include "{new}"'
            if old_pattern in content:
                content = content.replace(old_pattern, new_pattern)
                modified = True
                print(f"  {old} -> {new}")

        if modified:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(content)
            return True
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
    return False

def main():
    root_dirs = [
        'C:/Users/ryang/Documents/GitHub/gam300-magic/MagicEngine',
        'C:/Users/ryang/Documents/GitHub/gam300-magic/editor'
    ]

    extensions = {'.cpp', '.h', '.hpp', '.ipp'}
    modified_count = 0

    for root_dir in root_dirs:
        for dirpath, dirnames, filenames in os.walk(root_dir):
            for filename in filenames:
                ext = os.path.splitext(filename)[1].lower()
                if ext in extensions:
                    filepath = os.path.join(dirpath, filename)
                    if process_file(filepath):
                        print(f"Modified: {filepath}")
                        modified_count += 1

    print(f"\nTotal files modified: {modified_count}")

if __name__ == '__main__':
    main()
