package com.astra.injector;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.PixelFormat;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

public class OverlayService extends Service {
    private static final String CHANNEL = "astra_overlay";
    private WindowManager wm;
    private OverlayView overlay;
    private Handler handler = new Handler(Looper.getMainLooper());
    private Runnable frameRunner;

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();
        startForeground(1, buildNotification());

        // overlay view
        overlay = new OverlayView(this, null);
        WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                ? WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
                : WindowManager.LayoutParams.TYPE_SYSTEM_ALERT,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT
        );
        lp.gravity = Gravity.TOP | Gravity.START;
        wm = (WindowManager) getSystemService(WINDOW_SERVICE);
        if (checkOverlayPermission()) wm.addView(overlay, lp);

        // copy offsets from assets to files dir
        copyOffsetsFromAssets();

        // start native loop
        String version = getGameVersion();
        NativeBridge.init();
        NativeBridge.start("com.mobile.legends", version);

        // poll frames
        frameRunner = new Runnable() {
            @Override public void run() {
                Entity[] ents = NativeBridge.getFrame();
                if (ents != null) overlay.updateEntities(ents);
                LobbyEntry[] lobby = NativeBridge.getLobby();
                if (lobby != null) overlay.updateLobby(lobby);
                handler.postDelayed(this, 16);
            }
        };
        handler.post(frameRunner);
    }

    private void copyOffsetsFromAssets() {
        java.io.File dir = new java.io.File(getFilesDir(), "offsets");
        dir.mkdirs();
        java.io.File out = new java.io.File(dir, "offsets.json");
        try {
            java.io.InputStream in = getAssets().open("offsets/default.json");
            java.io.FileOutputStream fos = new java.io.FileOutputStream(out);
            byte[] buf = new byte[4096]; int n;
            while ((n = in.read(buf)) > 0) fos.write(buf, 0, n);
            in.close(); fos.close();
        } catch (Exception e) { e.printStackTrace(); }
    }

    private String getGameVersion() {
        try {
            return getPackageManager().getPackageInfo("com.mobile.legends", 0).versionName;
        } catch (PackageManager.NameNotFoundException e) { return "1.9.20.9876"; }
    }

    private boolean checkOverlayPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return android.provider.Settings.canDrawOverlays(this);
        }
        return true;
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel ch = new NotificationChannel(CHANNEL, "Astra Overlay",
                NotificationManager.IMPORTANCE_LOW);
            ((NotificationManager) getSystemService(NOTIFICATION_SERVICE)).createNotificationChannel(ch);
        }
    }

    private Notification buildNotification() {
        return new NotificationCompat.Builder(this, CHANNEL)
            .setContentTitle("Astra Injector")
            .setContentText("Overlay active")
            .setSmallIcon(android.R.drawable.ic_menu_info_details)
            .build();
    }

    @Override
    public void onDestroy() {
        handler.removeCallbacks(frameRunner);
        NativeBridge.stop();
        if (overlay != null && wm != null) wm.removeView(overlay);
        stopForeground(true);
        super.onDestroy();
    }

    @Nullable @Override
    public IBinder onBind(Intent intent) { return null; }
}