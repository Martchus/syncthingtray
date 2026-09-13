package io.github.martchus.syncthingtray;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.util.Log;

public class BootReceiver extends BroadcastReceiver {
    private static final String TAG = "BootReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) {
            return;
        }
        String action = intent.getAction();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action) || "android.intent.action.LOCKED_BOOT_COMPLETED".equals(action)) {
            Log.i(TAG, "Boot complete received");
            if (Util.shouldStartOnBoot(context)) {
                Log.i(TAG, "Starting Syncthing service on boot");
                Intent serviceIntent = new Intent(context, SyncthingService.class);
                try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        context.startForegroundService(serviceIntent);
                    } else {
                        context.startService(serviceIntent);
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Failed to start Syncthing service on boot: " + e.getMessage());
                }
            } else {
                Log.i(TAG, "Start on boot is disabled in settings");
            }
        }
    }
}
