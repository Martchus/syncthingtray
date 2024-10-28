package io.github.martchus.syncthingtray;

import android.app.Service;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.Bitmap;
import android.os.Build;
import android.util.Log;

import io.github.martchus.syncthingtray.SyncthingServiceBinder;

public class SyncthingService extends Service
{
    private final SyncthingServiceBinder m_binder = new SyncthingServiceBinder(this);
    private static final String TAG = "SyncthingService";
    private static final int s_notificationID = 1;
    private static NotificationManager s_notificationManager;
    private static NotificationChannel s_notificationChannel;
    private static Notification.Builder s_notificationBuilder;
    private static Notification s_notification;
    private static String s_notificationTitle = "Syncthing";
    private static String s_notificationText = "Initializing …";
    private static String s_notificationSubText = "";
    private static Bitmap s_notificationIcon = null;

    private void initializeNotificationManagement()  {
        if (s_notificationManager != null) {
            return;
        }

        s_notificationManager = (NotificationManager)this.getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            s_notificationChannel = new NotificationChannel(SyncthingService.class.getCanonicalName(), "Syncthing", NotificationManager.IMPORTANCE_DEFAULT);
            s_notificationChannel.enableLights(false);
            s_notificationChannel.enableVibration(false);
            s_notificationChannel.setSound(null, null);
            s_notificationChannel.setShowBadge(false);
            s_notificationChannel.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            s_notificationManager.createNotificationChannel(s_notificationChannel);
            s_notificationBuilder = new Notification.Builder(this, s_notificationChannel.getId());
        } else {
            s_notificationBuilder = new Notification.Builder(this);
        }

        s_notificationBuilder
            .setContentTitle(s_notificationTitle)
            .setContentText(s_notificationText)
            .setSubText(s_notificationSubText)
            .setSmallIcon(R.drawable.ic_stat_notify)
            .setColor(0xff169ad1)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setPriority(Notification.PRIORITY_MIN)
            .setDefaults(Notification.DEFAULT_SOUND)
            .setAutoCancel(false);
    }

    public static void updateNotification(String text, String subText, Bitmap icon) {
        if (text != null) {
            s_notificationText = text;
        }
        if (subText != null) {
            s_notificationSubText = subText;
        }
        if (icon != null) {
            s_notificationIcon = icon;
        }
        if (s_notificationBuilder == null) {
            return;
        }
        Log.i(TAG, "Updating notification: " + s_notificationText);
        s_notificationBuilder.setContentText(text).setSubText(subText);
        if (s_notificationIcon != null) {
            s_notificationBuilder.setLargeIcon(icon);
        }
        s_notification = s_notificationBuilder.build();
        s_notificationManager.notify(TAG, s_notificationID, s_notification);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        initializeNotificationManagement();
        Log.i(TAG, "Created service and notification");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        s_notificationManager.cancel(s_notificationID);
        Log.i(TAG, "Destroyed service and notification");
    }

    @Override
    public SyncthingServiceBinder onBind(Intent intent) {
        return m_binder;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        Log.i(TAG, "Putting service in foreground and showing notification");
        if (s_notification == null) {
            s_notification = s_notificationBuilder.build();
        }
        // FIXME: The nofication shown here shows up delayed and thus after updateNotification() is invoked.
        //        It fortunately doesn't replace the updated notification again but is added as additional
        //        notification.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(s_notificationID, s_notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(s_notificationID, s_notification);
        }
        return Service.START_STICKY;
    }
}
