package com.magicengine.kurorekishi;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import android.content.Context;
import android.content.res.AssetManager;

import org.fmod.FMOD;

public class MainActivity extends NativeActivity {
    static {
        System.loadLibrary("fmod");
        System.loadLibrary("fmodstudio");

        System.loadLibrary("MagicEngineAndroid");
    }
    public native void setVulkanLayerPath(String path);
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // --- VULKAN LAYER SETUP START ---
        String layerPath = setupVulkanLayers(this);
        if (layerPath != null) {
            // 2. Call the native function to pass the absolute path
            setVulkanLayerPath(layerPath);
        }

        try {
            FMOD.init(this);
            Log.d("MainActivity", "FMOD.init() succeeded");
        } catch (Exception e) {
            Log.e("MainActivity", "FMOD.init() failed: " + e.getMessage());
        }
        super.onCreate(savedInstanceState);
        Log.d("FMOD", "Java init called");
    }

    private String setupVulkanLayers(Context context) {
        final String LAYER_MANIFEST_FILENAME = "VkLayer_khronos_validation.json";
        final String LAYER_DIR_NAME = "vulkan_layers";

        try {
            File filesDir = context.getFilesDir();
            File layerDir = new File(filesDir, LAYER_DIR_NAME);
            if (!layerDir.exists()) layerDir.mkdirs();

            File manifestFile = new File(layerDir, LAYER_MANIFEST_FILENAME);
            if (!manifestFile.exists()) {
                AssetManager assetManager = context.getAssets();
                InputStream is = assetManager.open(LAYER_MANIFEST_FILENAME);
                FileOutputStream fos = new FileOutputStream(manifestFile);

                byte[] buffer = new byte[1024];
                int read;
                while ((read = is.read(buffer)) != -1) {
                    fos.write(buffer, 0, read);
                }
                fos.close();
                is.close();
                Log.d("Vulkan", "Copied Vulkan layer manifest to: " + manifestFile.getAbsolutePath());
            }

            // Return the absolute directory path
            return layerDir.getAbsolutePath();

        } catch (IOException e) {
            Log.e("Vulkan", "Failed to setup Vulkan layers: " + e.getMessage());
            return null;
        }
    }
}