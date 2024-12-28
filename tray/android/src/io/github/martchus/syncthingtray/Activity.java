package io.github.martchus.syncthingtray;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.net.Uri;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.widget.Toast;

import androidx.core.content.FileProvider;

import java.io.*;

import org.qtproject.qt.android.bindings.QtActivity;

import io.github.martchus.syncthingtray.SyncthingService;
import io.github.martchus.syncthingtray.Util;

public class Activity extends QtActivity {

    public boolean performHapticFeedback() {
        View rootView = getWindow().getDecorView().getRootView();
        boolean res = false;
        if (rootView != null) {
            rootView.setHapticFeedbackEnabled(true);
            res = rootView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }
        return res;
    }

    public boolean showToast(String message) {
        Activity activity = this;
        runOnUiThread(new Runnable() {
            public void run() {
                Toast.makeText(activity, message, Toast.LENGTH_LONG).show();
            }
        });
        return true;
    }

    public boolean openPath(String path) {
        // use FileProvider to compute an URI, using Uri.fromFile would lead to FileUriExposedException
        File file = new File(path);
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri fileUri = FileProvider.getUriForFile(this, getApplicationContext().getPackageName() + ".qtprovider", file);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        if (file.isDirectory()) {
            String absolutePath = "";
            try {
                absolutePath = file.getCanonicalPath();
            } catch (IOException e) {
                absolutePath = file.getAbsolutePath();
            }
            showToast(absolutePath);
            //intent.setDataAndType(fileUri, DocumentsContract.Document.MIME_TYPE_DIR);
            intent.setDataAndType(fileUri, "resource/folder");
            intent.putExtra("org.openintents.extra.ABSOLUTE_PATH", absolutePath);
        } else {
            intent.setDataAndType(fileUri, "application/*");
        }
        try {
            startActivity(intent);
        } catch (ActivityNotFoundException e1) {
            return false;
        }
        return true;
    }

    public String resolveUri(String uri) {
        return Util.getAbsolutePathFromStorageAccessFrameworkUri(this, Uri.parse(uri));
    }

    public void startSyncthingService() {
        Intent intent = new Intent(this, SyncthingService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    public void stopSyncthingService() {
        stopService(new Intent(this, SyncthingService.class));
    }

    void handleSendText(Intent intent) {
        String sharedText = intent.getStringExtra(Intent.EXTRA_TEXT);
        if (sharedText != null) {
            sendAndroidIntentToQtQuickApp("sharedtext:" + sharedText, false);
        }
    }

    public void onCreate (Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        String action = intent.getAction();
        String type = intent.getType();
        if (Intent.ACTION_SEND.equals(action) && "text/plain".equals(type)) {
            handleSendText(intent);
        }
    }

    protected void onNewIntent(Intent intent) {
        boolean fromNotification = intent.getBooleanExtra("notification", false);
        if (fromNotification) {
            sendAndroidIntentToQtQuickApp(intent.getStringExtra("page"), fromNotification);
        }
    }

    private void sendAndroidIntentToQtQuickApp(String page, boolean fromNotification) {
        try {
            onAndroidIntent(page, fromNotification);
        } catch (java.lang.UnsatisfiedLinkError e) {
            showToast("Unable to open in Syncthing Tray app right now.");
        }
    }

    private static native void onAndroidIntent(String page, boolean fromNotification);
}
