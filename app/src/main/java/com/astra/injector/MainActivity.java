package com.astra.injector;

import android.Manifest;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

public class MainActivity extends AppCompatActivity {
    private static final int OVERLAY_REQ = 1001;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(40, 80, 40, 40);

        Button b1 = new Button(this);
        b1.setText("Grant Overlay Permission");
        b1.setOnClickListener(v -> requestOverlay());
        root.addView(b1);

        Button b2 = new Button(this);
        b2.setText("Start Overlay Service");
        b2.setOnClickListener(v -> startOverlay());
        root.addView(b2);

        Button b3 = new Button(this);
        b3.setText("Stop Overlay Service");
        b3.setOnClickListener(v -> stopOverlay());
        root.addView(b3);

        setContentView(root);
    }

    private void requestOverlay() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            Intent i = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName()));
            startActivityForResult(i, OVERLAY_REQ);
        } else {
            Toast.makeText(this, "Overlay permission already granted", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OVERLAY_REQ) {
            Toast.makeText(this, Settings.canDrawOverlays(this) ? "Granted" : "Denied", Toast.LENGTH_SHORT).show();
        }
    }

    private void startOverlay() {
        Intent svc = new Intent(this, OverlayService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) startForegroundService(svc);
        else startService(svc);
        Toast.makeText(this, "Overlay started", Toast.LENGTH_SHORT).show();
    }

    private void stopOverlay() {
        stopService(new Intent(this, OverlayService.class));
        Toast.makeText(this, "Overlay stopped", Toast.LENGTH_SHORT).show();
    }
}