package io.github.martchus.syncthingtray;

import android.app.Service;
import android.app.PendingIntent;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.NotificationChannel;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.Bitmap;
import android.graphics.drawable.Icon;
import android.os.Build;
import android.util.Log;

import io.github.martchus.syncthingtray.Activity;
import io.github.martchus.syncthingtray.SyncthingServiceBinder;

public class SyncthingService extends Service
{
    private final SyncthingServiceBinder m_binder = new SyncthingServiceBinder(this);
    private static final String TAG = "SyncthingService";
    private static SyncthingService s_instance = null;
    private static final int s_notificationID = 1;
    private static final int s_activityIntentRequestCode = 1;
    private Intent m_notificationContentIntent;
    private NotificationManager m_notificationManager;
    private NotificationChannel m_notificationChannel;
    private Notification.Builder m_notificationBuilder;
    private Notification m_notification;
    private static String s_notificationTitle = "Syncthing";
    private static String s_notificationText = "Initializing …";
    private static String s_notificationSubText = "";
    private static Bitmap s_notificationIcon = null;

    private void initializeNotificationManagement()  {
        if (m_notificationManager != null) {
            return;
        }

        m_notificationManager = (NotificationManager)this.getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            m_notificationChannel = new NotificationChannel(SyncthingService.class.getCanonicalName(), "Syncthing", NotificationManager.IMPORTANCE_DEFAULT);
            m_notificationChannel.enableLights(false);
            m_notificationChannel.enableVibration(false);
            m_notificationChannel.setSound(null, null);
            m_notificationChannel.setShowBadge(false);
            m_notificationChannel.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            m_notificationManager.createNotificationChannel(m_notificationChannel);
            m_notificationBuilder = new Notification.Builder(this, m_notificationChannel.getId());
        } else {
            m_notificationBuilder = new Notification.Builder(this);
        }

        m_notificationContentIntent = new Intent(this, Activity.class);
        m_notificationContentIntent.putExtra("notification", true);

        m_notificationBuilder
            .setContentTitle(s_notificationTitle)
            .setContentText(s_notificationText)
            .setSubText(s_notificationSubText)
            .setSmallIcon(R.drawable.ic_stat_notify)
            .setColor(0xff169ad1)
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setContentIntent(PendingIntent.getActivity(this, s_activityIntentRequestCode, m_notificationContentIntent, PendingIntent.FLAG_IMMUTABLE))
            .setPriority(Notification.PRIORITY_MIN)
            .setDefaults(Notification.DEFAULT_SOUND)
            .setCategory(Notification.CATEGORY_SERVICE)
            .setGroup(TAG)
            .setForegroundServiceBehavior(Notification.FOREGROUND_SERVICE_IMMEDIATE)
            .setAutoCancel(false);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            m_notificationBuilder.setFlag(Notification.FLAG_NO_CLEAR | Notification.FLAG_FOREGROUND_SERVICE, true);
        }
    }

    public static void updateNotification(String text, String subText, Bitmap bitmapIcon) {
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
        Log.i(TAG, "Updating notification: " + s_notificationText);
        s_instance.m_notificationBuilder.setContentText(s_notificationText).setSubText(s_notificationSubText);
        if (s_notificationIcon != null) {
            Icon icon = Icon.createWithBitmap(s_notificationIcon);
            s_instance.m_notificationBuilder.setSmallIcon(icon).setLargeIcon(icon);
        }
        s_instance.m_notification = s_instance.m_notificationBuilder.build();
        s_instance.showForegroundNotification();
    }

    @Override
    public void onCreate() {
        super.onCreate();
        initializeNotificationManagement();
        s_instance = this;
        Log.i(TAG, "Created service and notification");
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopForeground(Service.STOP_FOREGROUND_REMOVE);
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
        showForegroundNotification();
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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(s_notificationID, m_notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(s_notificationID, m_notification);
        }
    }
}
