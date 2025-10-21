import os
import sys

# The root directory of your assets, relative to where the script is run
asset_dir = 'android/app/src/main/assets'
output_file = os.path.join(asset_dir, 'asset_manifest.txt')

# Check if the asset directory exists
if not os.path.exists(asset_dir):
    print(f"Error: Asset directory '{asset_dir}' does not exist.")
    print(f"Current working directory: {os.getcwd()}")
    sys.exit(1)

# Ensure the output directory exists
os.makedirs(asset_dir, exist_ok=True)

try:
    with open(output_file, 'w', encoding='utf-8') as f:
        file_count = 0
        
        # Recursively walk through the asset directory
        for root, dirs, files in os.walk(asset_dir):
            # Sort for consistent output
            files.sort()
            
            for name in files:
                # Skip the manifest file itself
                if name == 'asset_manifest.txt':
                    continue
                
                full_path = os.path.join(root, name)
                # Get the path relative to the asset_dir and normalize slashes to forward slashes
                relative_path = os.path.relpath(full_path, asset_dir).replace('\\', '/')
                f.write(relative_path + '\n')
                file_count += 1
        
    print(f"Generated asset manifest at: {output_file}")
    print(f"Total files indexed: {file_count}")
    
except Exception as e:
    print(f"Error writing manifest: {e}")
    sys.exit(1)