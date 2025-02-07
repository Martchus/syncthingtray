package io.github.martchus.syncthingtray;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.res.Configuration;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.net.Uri;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.widget.Toast;
import android.util.Log;

import androidx.core.content.FileProvider;

import java.io.*;

import org.qtproject.qt.android.bindings.QtActivity;

import io.github.martchus.syncthingtray.SyncthingService;
import io.github.martchus.syncthingtray.Util;

public class Activity extends QtActivity {
    private static final String TAG = "SyncthingActivity";
    private float m_fontScale = 1.0f;
    private int m_fontWeightAdjustment = 0;

    public boolean performHapticFeedback() {
        View rootView = getWindow().getDecorView().getRootView();
        boolean res = false;
        if (rootView != null) {
            rootView.setHapticFeedbackEnabled(true);
            res = rootView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }
        return res;
    }

    public boolean minimize() {
        runOnUiThread(new Runnable() {
            public void run() {
                moveTaskToBack(true);
            }
        });
        return true;
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
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        if (file.isDirectory()) {
            String absolutePath = "";
            try {
                absolutePath = file.getCanonicalPath();
            } catch (IOException e) {
                absolutePath = file.getAbsolutePath();
            }
            intent.setDataAndType(fileUri, DocumentsContract.Document.MIME_TYPE_DIR);
            intent.setDataAndType(Uri.fromFile(file), "resource/folder");
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

    public boolean scanPath(String path) {
        MediaScannerConnection.scanFile(this, new String[]{path}, null, new MediaScannerConnection.OnScanCompletedListener() {
            public void onScanCompleted(String path, Uri uri) {
                showToast("Rescan of " + path + " completed");
            }
        });
        showToast("Triggered rescan of " + path);
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
            Log.i(TAG, "Sending shared text to Qt Quick app");
            sendAndroidIntentToQtQuickApp("sharedtext:" + sharedText, false);
        }
    }

    public float fontScale() {
        return m_fontScale;
    }

    public int fontWeightAdjustment() {
        return m_fontWeightAdjustment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "Creating");

        // workaround https://github.com/golang/go/issues/70508
        System.loadLibrary("androidsignalhandler");
        initSigsysHandler();

        super.onCreate(savedInstanceState);

        // read font scale as Qt does not handle this automatically on Android
        Configuration config = getResources().getConfiguration();
        m_fontScale = config.fontScale;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            m_fontWeightAdjustment = config.fontWeightAdjustment;
        }

        // read text another app might have shared with us
        Intent intent = getIntent();
        String action = intent.getAction();
        String type = intent.getType();
        if (Intent.ACTION_SEND.equals(action) && "text/plain".equals(type)) {
            handleSendText(intent);
        }
    }

    @Override
    public void onStart() {
        Log.i(TAG, "Starting");
        super.onStart();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "Resuming");
        super.onResume();
    }

    @Override
    public void onPause() {
        Log.i(TAG, "Pausing");
        super.onPause();
    }

    public void onStop() {
        Log.i(TAG, "Stopping");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        // stop service and libsyncthing
        // note: QtActivity will exit the main thread in super.onDestroy() so we cannot keep the service running.
        //       It would not stop the service and Go threads but it would be impossible to re-enter the UI leaving
        //       the app in some kind of zombie state.
        Log.i(TAG, "Destroying");
        stopLibSyncthing();
        stopSyncthingService();
        super.onDestroy();
    }

    protected void onNewIntent(Intent intent) {
        boolean fromNotification = intent.getBooleanExtra("notification", false);
        if (fromNotification) {
            sendAndroidIntentToQtQuickApp(intent.getStringExtra("page"), fromNotification);
        }
    }

    private void sendAndroidIntentToQtQuickApp(String page, boolean fromNotification) {
        try {
            handleAndroidIntent(page, fromNotification);
        } catch (java.lang.UnsatisfiedLinkError e) {
            showToast("Unable to open in Syncthing Tray app right now.");
        }
    }

    private static native void handleAndroidIntent(String page, boolean fromNotification);
    private static native void stopLibSyncthing();
    private static native void initSigsysHandler();
}
