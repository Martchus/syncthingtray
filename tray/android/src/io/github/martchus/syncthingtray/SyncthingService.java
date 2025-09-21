package io.github.martchus.syncthingtray;

import android.app.ForegroundServiceStartNotAllowedException;
import android.app.Service;
import android.app.PendingIntent;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.Icon;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import java.util.ArrayList;

import org.qtproject.qt.android.bindings.QtService;

import io.github.martchus.syncthingtray.Activity;
import io.github.martchus.syncthingtray.Util;

public class SyncthingService extends QtService {
    private static final String TAG = "SyncthingService";
    private static SyncthingService s_instance = null;

    // fields for managing notifications
    private static final int s_notificationID = 1;
    private static final int s_activityIntentRequestCode = 1;
    private static final int s_serviceIntentRequestCode = 1;
    private Intent m_notificationContentIntent;
    private Intent m_shutdownIntent;
    private NotificationManager m_notificationManager;
    private NotificationChannel m_notificationChannel;
    private NotificationChannel m_extraNotificationChannel;
    private Notification.Builder m_notificationBuilder;
    private Notification.Builder m_extraNotificationBuilder;
    private Notification m_notification;
    private static String s_notificationTitle = "Syncthing";
    private static String s_notificationText = "Initializing …";
    private static String s_notificationSubText = "";
    private static Bitmap s_notificationIcon = null;
    private String m_locale = "";

    // fields to communicate with activity
    private ArrayList<Messenger> m_clients = new ArrayList<Messenger>();
    private final Messenger m_messenger = new Messenger(new IncomingHandler());

    // messages to register/unregister clients (used from Java only)
    public static final int MSG_REGISTER_CLIENT = 1;
    public static final int MSG_UNREGISTER_CLIENT = 2;
    public static final int MSG_FINISH_CLIENT = 3;
    // messages to invoke activity and service actions invoked from Java and C++ (keep in sync with android.h)
    public static final int MSG_SERVICE_ACTION_BROADCAST_LAUNCHER_STATUS = 105;

    private class IncomingHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_REGISTER_CLIENT:
                m_clients.add(msg.replyTo);
                break;
            case MSG_UNREGISTER_CLIENT:
                m_clients.remove(msg.replyTo);
                break;
            default:
                Bundle bundle = msg.getData();
                String str = bundle != null ? bundle.getString("message") : null;
                try {
                    handleMessageFromActivity(msg.what, msg.arg1, msg.arg2, str);
                } catch (java.lang.UnsatisfiedLinkError e) {
                    Log.i(TAG, "Unable to handle message " + msg.what + " from activity, backend not started yet");
                }
                super.handleMessage(msg);
            }
        }
    }

    public static Message obtainMessageWithBundle(int what, int arg1, int arg2, Bundle bundle) {
        Message msg = Message.obtain(null, what, arg1, arg2);
        if (bundle != null) {
            msg.setData(bundle);
        }
        return msg;
    }

    public static Bundle bundleString(String str) {
        if (str == null) {
            return null;
        }
        Bundle bundle = new Bundle();
        bundle.putString("message", str);
        return bundle;
    }

    public static Bundle bundleVariant(byte[] variant) {
        if (variant == null) {
            return null;
        }
        Bundle bundle = new Bundle();
        bundle.putByteArray("variant", variant);
        return bundle;
    }

    public int sendMessageToClients(int what, int arg1, int arg2, Bundle data) {
        int messagesSent = 0;
        for (int i = m_clients.size() - 1; i >= 0; --i) {
            try {
                m_clients.get(i).send(obtainMessageWithBundle(what, arg1, arg2, data));
                ++messagesSent;
            } catch (RemoteException e) {
                m_clients.remove(i); // remove presumably dead client
            }
        }
        return messagesSent;
    }

    public int sendMessageToClients(int what, int arg1, int arg2, String str) {
        return sendMessageToClients(what, arg1, arg2, bundleString(str));
    }

    public int sendMessageToClients(int what, int arg1, int arg2, byte[] variant) {
        return sendMessageToClients(what, arg1, arg2, bundleVariant(variant));
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "Returning messenger binder");
        return m_messenger.getBinder();
    }
    
    private void initializeNotificationManagement()  {
        if (m_notificationManager != null) {
            return;
        }

        m_notificationManager = (NotificationManager)this.getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // add notification channel for persistent notification tied to service
            m_notificationChannel = new NotificationChannel(SyncthingService.class.getCanonicalName(), "Syncthing", NotificationManager.IMPORTANCE_DEFAULT);
            m_notificationChannel.enableLights(false);
            m_notificationChannel.enableVibration(false);
            m_notificationChannel.setSound(null, null);
            m_notificationChannel.setShowBadge(false);
            m_notificationChannel.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            m_notificationManager.createNotificationChannel(m_notificationChannel);
            m_notificationBuilder = new Notification.Builder(this, m_notificationChannel.getId());
            // add notification channel for extra notifications
            m_extraNotificationChannel = new NotificationChannel(SyncthingService.class.getCanonicalName(), "Syncthing notifications", NotificationManager.IMPORTANCE_MIN);
            m_extraNotificationChannel.enableLights(false);
            m_extraNotificationChannel.enableVibration(true);
            m_extraNotificationChannel.setShowBadge(false);
            m_extraNotificationChannel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
            m_notificationManager.createNotificationChannel(m_extraNotificationChannel);
            m_extraNotificationBuilder = new Notification.Builder(this, m_extraNotificationChannel.getId());
        } else {
            m_notificationBuilder = new Notification.Builder(this);
            m_extraNotificationBuilder = new Notification.Builder(this);
        }

        m_notificationContentIntent = new Intent(this, Activity.class);
        m_notificationContentIntent.putExtra("notification", true);
        m_shutdownIntent = new Intent(this, SyncthingService.class);
        m_shutdownIntent.setAction("shutdown");

        m_notificationBuilder
            .setContentTitle(s_notificationTitle)
            .setContentText(s_notificationText)
            .setSubText(s_notificationSubText)
            .setSmallIcon(R.drawable.ic_stat_notify)
            .setColor(0xff169ad1)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setContentIntent(PendingIntent.getActivity(this, s_activityIntentRequestCode, m_notificationContentIntent, PendingIntent.FLAG_IMMUTABLE))
            .addAction((new Notification.Action.Builder(null, "Shutdown", PendingIntent.getService(this, s_serviceIntentRequestCode, m_shutdownIntent, PendingIntent.FLAG_IMMUTABLE)).build()))
            .setPriority(Notification.PRIORITY_DEFAULT)
            .setDefaults(Notification.DEFAULT_SOUND)
            .setCategory(Notification.CATEGORY_SERVICE)
            .setGroup(TAG)
            .setAutoCancel(false);
        m_extraNotificationBuilder
            .setSmallIcon(R.drawable.ic_stat_notify)
            .setColor(0xff169ad1)
            .setPriority(Notification.PRIORITY_MIN)
            .setDefaults(Notification.DEFAULT_SOUND)
            .setCategory(Notification.CATEGORY_SERVICE)
            .setGroup(TAG);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            m_notificationBuilder.setForegroundServiceBehavior(Notification.FOREGROUND_SERVICE_IMMEDIATE);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            m_notificationBuilder.setFlag(Notification.FLAG_NO_CLEAR | Notification.FLAG_FOREGROUND_SERVICE, true);
        }
    }

    public static boolean areNotificationsEnabled() {
        return s_instance != null && s_instance.m_notificationManager.areNotificationsEnabled();
    }

    public static void updateNotification(String title, String text, String subText, Bitmap bitmapIcon) {
        if (s_notificationTitle.equals(title) && s_notificationText.equals(text) && s_notificationSubText.equals(subText)) {
            return;
        }
        if (title != null) {
            s_notificationTitle = title;
        }
        if (text != null) {
            s_notificationText = text;
        }
        if (subText != null) {
            s_notificationSubText = subText;
        }
        if (bitmapIcon != null) {
            s_notificationIcon = bitmapIcon;
        }
        if (s_instance == null) {
            return;
        }
        Log.i(TAG, "Updating notification: " + s_notificationTitle + " - " + s_notificationText);
        s_instance.m_notificationBuilder.setContentTitle(s_notificationTitle).setContentText(s_notificationText).setSubText(s_notificationSubText);
        if (s_notificationIcon != null) {
            // update only large icon (not calling setSmallIcon(icon)) because Android doesn't render the badge correctly on the small icon
            Icon icon = Icon.createWithBitmap(s_notificationIcon);
            s_instance.m_notificationBuilder.setLargeIcon(icon);
        }
        s_instance.m_notification = s_instance.m_notificationBuilder.build();
        s_instance.showForegroundNotification();
    }

    public static void updateExtraNotification(String title, String text, String subText, String page, Bitmap bitmapIcon, int id) {
        if (s_instance == null) {
            return;
        }
        Log.i(TAG, "Showing extra notification: " + text);
        Intent intent = new Intent(s_instance, Activity.class);
        intent.putExtra("notification", true);
        intent.putExtra("page", page);
        s_instance.m_extraNotificationBuilder.setContentTitle(title).setContentText(text).setSubText(subText);
        s_instance.m_extraNotificationBuilder.setSmallIcon(bitmapIcon != null ? Icon.createWithBitmap(bitmapIcon) : null);
        s_instance.m_extraNotificationBuilder.setContentIntent(PendingIntent.getActivity(s_instance, s_activityIntentRequestCode + id, intent, PendingIntent.FLAG_IMMUTABLE));
        s_instance.m_notificationManager.notify(id, s_instance.m_extraNotificationBuilder.build());
    }

    public static void cancelExtraNotification(int firstID, int lastID) {
        if (s_instance == null) {
            return;
        }
        if (firstID > lastID) {
            s_instance.m_notificationManager.cancel(firstID);
            return;
        }
        for (; firstID != lastID; ++firstID) {
            s_instance.m_notificationManager.cancel(firstID);
        }
    }

    @Override
    public void onCreate() {
        Log.i(TAG, "Creating Syncthing service");
        Util.init();

        Log.i(TAG, "Putting service in foreground and showing notification");
        initializeNotificationManagement();
        showForegroundNotification();
        s_instance = this;

        // blocks until QAndroidService::exec() is entered so AppService c'tor has run after this line
        // note: This seems broken as of commit d00e5433f6b4d91d481e1a4f7a193d0f5b4ff10c on qtbase dev branch
        //       leading to super.onCreate() returning immediately and the native main() function never be
        //       called as a previous call to QtAndroidPrivate::waitForServiceSetup() blocks indefinitely.
        //       Hence the service code is prepared for all native function calls to fail with an
        //       UnsatisfiedLinkError exception. Not being able to handle these calls on early startup does
        //       not seem to break anything.
        super.onCreate();
        Log.i(TAG, "Created service and notification");
    }

    @Override
    public void onDestroy() {
        s_instance = null;
        try {
            super.onDestroy();
            stopLibSyncthing();
        } catch (java.lang.UnsatisfiedLinkError e) {
            Log.i(TAG, "Unable to destroy, backend not started anyway");
        }
        stopForeground(Service.STOP_FOREGROUND_REMOVE);
        Log.i(TAG, "Destroyed service and notification");
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        m_locale = intent.getStringExtra("locale");
        super.onStartCommand(intent, flags, startId);
        if (intent != null && "shutdown".equals(intent.getAction())) {
            sendMessageToClients(MSG_FINISH_CLIENT, 0, 0, "");
            stopForeground(Service.STOP_FOREGROUND_REMOVE);
            stopSelf();
        } else {
            try {
                broadcastLauncherStatus();
            } catch (java.lang.UnsatisfiedLinkError e) {
                Log.i(TAG, "Unable to broadcast launcher status, backend not started yet");
            }
        }
        return Service.START_STICKY;
    }

    public void showForegroundNotification() {
        if (m_notification == null) {
            m_notification = m_notificationBuilder.build();
        }

        // use startForeground() to make sure the service gets CPU time even when the app is hidden
        // note: This also shows the notification. Invoking notificationManager.notify() would lead to duplicate notifications.
        //       We also have to use startForeground() again to update the notification because invoking notificationManager.notify()
        //       would not update the initial notification and instead again lead to duplicate notifications.
        //       We also have to use stopForeground() to remove the notification because invoking notificationManager.cancel() doesn't
        //       have any effect.
        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                startForeground(s_notificationID, m_notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
            } else {
                startForeground(s_notificationID, m_notification);
            }
            cancelExtraNotification(s_notificationID, -1);
        } catch (ForegroundServiceStartNotAllowedException e) {
            updateExtraNotification(
                s_notificationTitle,
                "Unable restart foreground service to run Syncthing while app is in background. Click to start Syncthing app again.",
                "",
                "",
                null,
                s_notificationID);
        }
    }

    public float scaleFactor() {
        return Resources.getSystem().getDisplayMetrics().density;
    }

    public boolean isDarkmodeEnabled() {
        return (getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;
    }

    public boolean isNetworkConnectionMetered() {
        return ((ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE)).isActiveNetworkMetered();
    }

    public String getGatewayIPv4() {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? Util.getGatewayIPv4(this) : null;
    }

    public String getLocale() {
        return m_locale;
    }

    private static native void stopLibSyncthing();
    private static native void broadcastLauncherStatus();
    private static native void handleMessageFromActivity(int what, int arg1, int arg2, String str);
}
