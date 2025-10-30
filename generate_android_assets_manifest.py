import os
import sys

# The root directory of your assets, relative to where the script is run
android_asset_dir = 'android/app/src/main/assets'
output_file = os.path.join(android_asset_dir, 'asset_manifest.txt')
working_asset_dir = 'assets'



def rename_to_lowercase(directory):
    """Recursively rename all files and directories to lowercase."""
    renamed_count = 0
    
    # Walk bottom-up to handle nested directories properly
    for root, dirs, files in os.walk(directory, topdown=False):
        # Rename files first
        for name in files:
            if name == 'asset_manifest.txt':
                continue
                
            lowercase_name = name.lower()
            if name != lowercase_name:
                old_path = os.path.join(root, name)
                new_path = os.path.join(root, lowercase_name)
                
                # Handle case where target file already exists (and is not the same file)
                if os.path.exists(new_path) and os.path.normcase(old_path) != os.path.normcase(new_path):
                    print(f"Warning: Cannot rename '{name}' to '{lowercase_name}' - target already exists")
                else:
                    os.rename(old_path, new_path)
                    print(f"Renamed: {name} -> {lowercase_name}")
                    renamed_count += 1
        
        # Rename directories
        for name in dirs:
            lowercase_name = name.lower()
            if name != lowercase_name:
                old_path = os.path.join(root, name)
                new_path = os.path.join(root, lowercase_name)
                
                if os.path.exists(new_path) and os.path.normcase(old_path) != os.path.normcase(new_path):
                    print(f"Warning: Cannot rename directory '{name}' to '{lowercase_name}' - target already exists")
                else:
                    os.rename(old_path, new_path)
                    print(f"Renamed directory: {name} -> {lowercase_name}")
                    renamed_count += 1
    
    return renamed_count





# Rename all assets to lowercase
print("Renaming assets to lowercase...")
renamed_count = rename_to_lowercase(working_asset_dir)
print(f"Renamed {renamed_count} files/directories to lowercase\n")




# Check if the asset directory exists
if not os.path.exists(android_asset_dir):
    print(f"Error: Asset directory '{android_asset_dir}' does not exist.")
    print(f"Current working directory: {os.getcwd()}")
    sys.exit(1)

# Ensure the output directory exists
os.makedirs(android_asset_dir, exist_ok=True)


# Building the asset manifest
try:
    with open(output_file, 'w', encoding='utf-8') as f:
        file_count = 0
        
        # Recursively walk through the asset directory
        for root, dirs, files in os.walk(working_asset_dir):
            # Sort for consistent output
            files.sort()
            
            for name in files:
                # Skip the manifest file itself
                if name == 'asset_manifest.txt':
                    continue
                
                full_path = os.path.join(root, name)
                # Get the path relative to the asset_dir and normalize slashes to forward slashes
                relative_path = os.path.relpath(full_path, working_asset_dir).replace('\\', '/').lower()
                f.write(relative_path + '\n')
                file_count += 1
        
    print(f"Generated asset manifest at: {output_file}")
    print(f"Total files indexed: {file_count}")
    
except Exception as e:
    print(f"Error writing manifest: {e}")
    sys.exit(1)