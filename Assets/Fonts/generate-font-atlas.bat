@echo off
setlocal enabledelayedexpansion

:: Loop through all .otf and .ttf files in the current directory
for %%f in (*.otf *.ttf) do (
    :: Extract the font name without extension for the output files
    set "fontname=%%~nf"

    :: Check if either the .png or .json file does not exist
    if not exist "!fontname!-atlas.png" if not exist "!fontname!-atlas.json" (
        :: Run the msdf-atlas-gen command if one or both files don't exist
        .\msdf-atlas-gen.exe -font "%%f" -imageout "!fontname!-atlas.png" -json "!fontname!-atlas.json" -format png -type mtsdf -pxrange 4 -uniformgrid
        echo Processed %%f
    ) else (
        echo Skipping %%f, atlas already exists.
    )
)

endlocal
