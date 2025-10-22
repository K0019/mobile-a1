package com.magicengine.kurorekishi;

import android.app.NativeActivity;
import android.os.Bundle;
import org.fmod.FMOD;

public class MainActivity extends NativeActivity {
    static {
        System.loadLibrary("fmod");
        System.loadLibrary("fmodstudio");
    }
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        FMOD.init(this);
        super.onCreate(savedInstanceState);
    }
}