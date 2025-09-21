package io.github.martchus.syncthingtray;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.Manifest;
import android.media.MediaScannerConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.provider.DocumentsContract;
import android.net.Uri;
import android.provider.Settings;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;

import net.lingala.zip4j.ZipFile;
import net.lingala.zip4j.exception.ZipException;
import net.lingala.zip4j.model.ExcludeFileFilter;
import net.lingala.zip4j.model.ZipParameters;
import net.lingala.zip4j.model.enums.CompressionLevel;
import net.lingala.zip4j.model.enums.CompressionMethod;
import net.lingala.zip4j.model.enums.EncryptionMethod;
import net.lingala.zip4j.model.enums.AesKeyStrength;

import java.io.*;

import org.qtproject.qt.android.bindings.QtActivity;

import io.github.martchus.syncthingtray.SyncthingService;
import io.github.martchus.syncthingtray.Util;

public class Activity extends QtActivity {
    private static final String TAG = "SyncthingActivity";

    // various fields for activity-internal state
    private static final int STORAGE_PERMISSION_REQUEST = 100;
    private float m_fontScale = 1.0f;
    private int m_fontWeightAdjustment = 0;
    private boolean m_storagePermissionRequested = false;
    private boolean m_notificationPermissionRequested = false;
    private boolean m_restarting = false;
    private boolean m_explicitShutdown = false;
    private boolean m_keepRunningAfterDestruction = false;
    private android.content.pm.ActivityInfo m_info;
    private String m_showPage = null;
    private boolean m_showFromNotification = false;
    private String m_locale = "";

    // fields for communicating with service
    private Messenger m_service = null;
    private boolean m_isBound;
    private final Messenger m_messenger = new Messenger(new IncomingHandler());

    private class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case SyncthingService.MSG_FINISH_CLIENT:
                m_explicitShutdown = true;
                finish();
                break;
            default:
                Bundle bundle = msg.getData();
                String str = null;
                byte[] variant = null;
                if (bundle != null) {
                    str = bundle.getString("message");
                    variant = bundle.getByteArray("variant");
                }
                handleMessageFromService(msg.what, msg.arg1, msg.arg2, str, variant);
            }
            super.handleMessage(msg);
        }
    }

    private ServiceConnection m_connection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className,
                                       IBinder service) {
            Log.i(TAG, "Connected to service");
            m_service = new Messenger(service);
            try {
                Message msg = Message.obtain(null, SyncthingService.MSG_REGISTER_CLIENT);
                msg.replyTo = m_messenger;
                m_service.send(msg);
                sendMessageToService(SyncthingService.MSG_SERVICE_ACTION_BROADCAST_LAUNCHER_STATUS, 0, 0, "");
            } catch (RemoteException e) {
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            Log.i(TAG, "Disconnected from service, trying to restart");
            m_service = null;
            if (!Activity.this.isFinishing()) {
                startSyncthingService();
                connectToService();
            }
        }
    };

    private void connectToService() {
        if (!m_isBound) {
            Log.i(TAG, "Connecting to service");
            bindService(new Intent(Activity.this, SyncthingService.class), m_connection, Context.BIND_AUTO_CREATE);
            m_isBound = true;
        }
    }

    public void sendMessageToService(int what, int arg1, int arg2, String str) {
        try {
            m_service.send(SyncthingService.obtainMessageWithBundle(what, arg1, arg2, SyncthingService.bundleString(str)));
        } catch (RemoteException e) {
            showToast("Unable to send message to background service: " + e.toString());
        }
    }

    private void disconnectFromService() {
        if (!m_isBound) {
            return;
        }
        Log.i(TAG, "Disconnecting from service");
        if (m_service != null) {
            try {
                Message msg = Message.obtain(null, SyncthingService.MSG_UNREGISTER_CLIENT);
                msg.replyTo = m_messenger;
                m_service.send(msg);
            } catch (RemoteException e) {
            }
        }
        unbindService(m_connection);
        m_isBound = false;
    }

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

    public void setLocale(String locale) {
        m_locale = locale;
    }

    public void startSyncthingService() {
        Intent intent = new Intent(this, SyncthingService.class);
        intent.putExtra("locale", m_locale);
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

    private ZipFile makeZipFile(String path, String password) {
        ZipFile zipFile = password.isEmpty()
            ? new ZipFile(path)
            : new ZipFile(path, password.toCharArray());
        zipFile.setUseUtf8CharsetForPasswords(true);
        return zipFile;
    }

    public String compressArchive(String settingsPath, String syncthingHomePath, String destinationArchivePath, String password) {
        try {
            ZipParameters parameters = new ZipParameters();
            parameters.setCompressionMethod(CompressionMethod.DEFLATE);
            parameters.setCompressionLevel(CompressionLevel.NORMAL);
            parameters.setExcludeFileFilter(new ExcludeFileFilter() {
                @Override
                public boolean isExcluded(File file) {
                    return file.getName().equals("syncthing.socket");
                }
            });
            ZipFile zipFile = makeZipFile(destinationArchivePath, password);
            if (!password.isEmpty()) {
                parameters.setEncryptFiles(true);
                parameters.setEncryptionMethod(EncryptionMethod.AES);
                parameters.setAesKeyStrength(AesKeyStrength.KEY_STRENGTH_256);
            }
            zipFile.addFolder(new File(settingsPath), parameters);
            if (!syncthingHomePath.isEmpty()) {
                parameters.setRootFolderNameInZip("syncthing");
                zipFile.addFolder(new File(syncthingHomePath), parameters);
            }
            return "";
        } catch (ZipException e) {
            return e.toString();
        }
    }

    public String extractArchive(String sourceArchivePath, String destinationDirectoryPath, String password)  {
        try {
            ZipFile zipFile = makeZipFile(sourceArchivePath, password);
            if (!password.isEmpty() && !zipFile.isEncrypted()) {
                zipFile = makeZipFile(sourceArchivePath, "");
            }
            zipFile.extractAll(destinationDirectoryPath);
            return "";
        } catch (ZipException e) {
            return e.toString();
        }
    }

    private void applyTheming() {
        // set color to Material.LightBlue from Material.Light, in consistency with MainToolBar.qml/Theming.qml
        // note: The status/navigation bar color cannot be set anymore like this under newer Android versions. So
        //       Qt.ExpandedClientAreaHint is used instead. However, this code still seems to do *something*. With
        //       it the icons in the status bar are rendered in white (instead of black) which looks much better.
        int color = 0xFF03A9F4;
        Window window = getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
        window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
        window.setStatusBarColor(color);
        window.setNavigationBarColor(color);
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

        applyTheming();
        Util.init();

        super.onCreate(savedInstanceState); // does *not* block, native code registering JNI functions will only run once layout is initialized

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

    public void onNativeReady(String locale) {
        m_locale = locale;
        startSyncthingService();
        connectToService();
    }

    public void onGuiLoaded() {
        if (m_showPage != null) {
            handleAndroidIntent(m_showPage, m_showFromNotification);
            m_showPage = null;
            m_showFromNotification = false;
        }
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
            startSyncthingService();
            connectToService();
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

        // stop only UI if BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION is configured
        // note: With this configuration the main UI process can keep running even after the activity has
        //       been destroyed. It can resume the UI once the activity is created again. This requires
        //       custom Qt patches on my qtbase fork. With these patches background_running_after_destruction
        //       can be enabled (via the CMake variable BACKGROUND_RUNNING_AFTER_ANDROID_ACTIVITY_DESTRUCTION).
        if (m_explicitShutdown) {
            m_keepRunningAfterDestruction = false;
        } else {
            Bundle md = metaData();
            m_keepRunningAfterDestruction = md != null && md.getBoolean("android.app.background_running_after_destruction");
        }
        Log.i(TAG, "Stopping Qt Quick GUI" + (m_keepRunningAfterDestruction ? ", keeping app running" : ""));
        unloadQtQuickGui();
        disconnectFromService();
        super.onDestroy();
    }

    @Override
    protected void onNewIntent(@NonNull Intent intent) {
        if (intent.getBooleanExtra("notification", false)) {
            sendAndroidIntentToQtQuickApp(intent.getStringExtra("page"), true);
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
            m_showPage = page;
            m_showFromNotification = fromNotification;
        }
    }

    private static native void loadQtQuickGui();
    private static native void unloadQtQuickGui();
    private static native void handleLauncherStatusBroadcast(Intent intent);
    private static native void handleMessageFromService(int what, int arg1, int arg2, String str, byte[] variant);
    private static native void handleAndroidIntent(String page, boolean fromNotification);
    private static native void handleStoragePermissionChanged(boolean storagePermissionGranted);
    private static native void handleNotificationPermissionChanged(boolean notificationPermissionGranted);
}
