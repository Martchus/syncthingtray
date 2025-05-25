package io.github.martchus.syncthingtray;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.Manifest;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.net.Uri;
import android.provider.Settings;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.widget.Toast;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;

import java.io.*;

import org.qtproject.qt.android.bindings.QtActivity;

import io.github.martchus.syncthingtray.SyncthingService;
import io.github.martchus.syncthingtray.Util;

public class Activity extends QtActivity {
    private static final String TAG = "SyncthingActivity";
    private static final int STORAGE_PERMISSION_REQUEST = 100;
    private float m_fontScale = 1.0f;
    private int m_fontWeightAdjustment = 0;
    private boolean m_storagePermissionRequested = false;
    private boolean m_notificationPermissionRequested = false;
    private boolean m_restarting = false;
    private boolean m_explicitShutdown = false;
    private boolean m_keepRunningAfterDestruction = false;
    private android.content.pm.ActivityInfo m_info;

    public boolean performHapticFeedback() {
        View rootView = getWindow().getDecorView().getRootView();
        boolean res = false;
        if (rootView != null) {
            rootView.setHapticFeedbackEnabled(true);
            res = rootView.performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
        }
        return res;
    }

    public boolean isExternalStorageMounted() {
        return Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
    }

    public String externalFilesDir() {
        return getExternalFilesDir(null).getAbsolutePath();
    }

    public String[] externalStoragePaths() {
        File[] files = ContextCompat.getExternalFilesDirs(getApplicationContext(), null);
        String[] paths = new String[files.length];
        for (int i = 0, len = files.length; i != len; ++i) {
            paths[i] = files[i].getAbsolutePath();
        }
        return paths;
    }

    public boolean storagePermissionGranted() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
    }

    public boolean requestStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
            intent.setData(Uri.parse("package:" + getPackageName()));
            try {
                startActivity(intent);
                m_storagePermissionRequested = true;
                return true;
            } catch (ActivityNotFoundException ignored) {
                showToast("Unable to request storage permission.");
            }
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE}, STORAGE_PERMISSION_REQUEST);
        }
        return false;
    }

    public boolean notificationPermissionGranted() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            return checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED;
        } else {
            return true;
        }
    }

    public boolean requestNotificationPermission() {
        Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        intent.setData(Uri.parse("package:" + getPackageName()));
        try {
            startActivity(intent);
            m_notificationPermissionRequested = true;
            return true;
        } catch (ActivityNotFoundException ignored) {
            showToast("Unable to request notification permission.");
            return false;
        }
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
        m_explicitShutdown = true;
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

    private Bundle metaData() {
        if (m_info == null) {
            try {
                m_info = getPackageManager().getActivityInfo(getComponentName(), PackageManager.GET_META_DATA);
            } catch (PackageManager.NameNotFoundException e) {
                Log.e(TAG, "Unable to get activity info: " + e.toString());
                return null;
            }
        }
        return m_info.metaData;
    }

    //@Override: overrides if patched version of Qt is used
    protected boolean handleRestart(Bundle savedInstanceState) {
        Log.i(TAG, "Restarting");
        return m_restarting = true;
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
    }

    @Override
    public void onStart() {
        Log.i(TAG, "Starting");
        super.onStart();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "Resuming");
        if (m_storagePermissionRequested) {
            m_storagePermissionRequested = false;
            handleStoragePermissionChanged(storagePermissionGranted());
        }
        if (m_notificationPermissionRequested) {
            m_notificationPermissionRequested = false;
            handleNotificationPermissionChanged(notificationPermissionGranted());
        }
        super.onResume();

        // load the Qt Quick GUI again when the activity was previously destroyed
        if (m_restarting) {
            m_restarting = false;
            loadQtQuickGui();
        }

        // read text another app might have shared with us
        Intent intent = getIntent();
        String action = intent.getAction();
        String type = intent.getType();
        if (Intent.ACTION_SEND.equals(action) && "text/plain".equals(type)) {
            handleSendText(intent);
        } else {
            onNewIntent(intent);
        }
    }

    @Override
    public void onPause() {
        Log.i(TAG, "Pausing");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "Stopping");
        super.onStop();
    }

    //@Override: overrides if patched version of Qt is used
    protected boolean handleDestruction() {
        return m_keepRunningAfterDestruction;
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "Destroying");

        // stop service and libsyncthing unless background_running_after_destruction is configured
        // note: QtActivity will normally exit the main thread in super.onDestroy() so we cannot keep the service running.
        //       It would not stop the service and Go threads but it would be impossible to re-enter the UI leaving the app
        //       in some kind of zombie state. This is fixed by custom Qt patches on my qtbase fork. With these patches
        //       background_running_after_destruction can be enabled (via the CMake variable
        //       BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION) as the problematic default behavior of Qt is
        //       prevented.
        if (m_explicitShutdown) {
            m_keepRunningAfterDestruction = false;
        } else {
            Bundle md = metaData();
            m_keepRunningAfterDestruction = md != null && md.getBoolean("android.app.background_running_after_destruction");
        }
        if (m_keepRunningAfterDestruction) {
            Log.i(TAG, "Stopping only Qt Quick GUI");
            unloadQtQuickGui();
        } else {
            Log.i(TAG, "Stopping Syncthing and Qt Quick GUI");
            stopLibSyncthing();
            stopSyncthingService();
        }
        super.onDestroy();
    }

    @Override
    protected void onNewIntent(@NonNull Intent intent) {
        if (intent.getBooleanExtra("notification", false)) {
            sendAndroidIntentToQtQuickApp(intent.getStringExtra("page"), true);
        } else if ("shutdown".equals(intent.getAction())) {
            stopSyncthingService();
            finish();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == STORAGE_PERMISSION_REQUEST) {
            handleStoragePermissionChanged(grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED);
        }
    }

    private void sendAndroidIntentToQtQuickApp(String page, boolean fromNotification) {
        try {
            handleAndroidIntent(page, fromNotification);
        } catch (java.lang.UnsatisfiedLinkError e) {
            showToast("Unable to open in Syncthing Tray app right now.");
        }
    }

    private static native void loadQtQuickGui();
    private static native void unloadQtQuickGui();
    private static native void handleAndroidIntent(String page, boolean fromNotification);
    private static native void handleStoragePermissionChanged(boolean storagePermissionGranted);
    private static native void handleNotificationPermissionChanged(boolean notificationPermissionGranted);
    private static native void stopLibSyncthing();
    private static native void initSigsysHandler();
}
