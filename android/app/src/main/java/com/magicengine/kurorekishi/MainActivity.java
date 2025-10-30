package com.magicengine.kurorekishi;

import android.app.NativeActivity;
import android.os.Bundle;
import android.util.Log;

import org.fmod.FMOD;

public class MainActivity extends NativeActivity {
    static {
        System.loadLibrary("fmod");
        System.loadLibrary("fmodstudio");
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        try {
            FMOD.init(this);
            Log.d("MainActivity", "FMOD.init() succeeded");
        } catch (Exception e) {
            Log.e("MainActivity", "FMOD.init() failed: " + e.getMessage());
        }
        super.onCreate(savedInstanceState);
        Log.d("FMOD", "Java init called");
    }
}