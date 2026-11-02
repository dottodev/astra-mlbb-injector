package com.astra.injector;

import android.app.Application;
import android.content.Intent;
import android.os.Build;

public class AstraApp extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(new Intent(this, OverlayService.class));
        } else {
            startService(new Intent(this, OverlayService.class));
        }
    }
}