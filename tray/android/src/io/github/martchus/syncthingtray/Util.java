/*
 * This code is based on com.nutomic.syncthingandroid.util from:
 * https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java
 * https://github.com/researchxxl/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java
 */

package io.github.martchus.syncthingtray;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.Network;
import android.net.RouteInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.File;
import java.lang.IllegalArgumentException;
import java.lang.reflect.Array;
import java.lang.reflect.Method;
import java.net.Inet4Address;
import java.net.InetAddress;

public class Util {
    private static final String TAG = "Util";
    private static final String DOWNLOADS_VOLUME_NAME = "downloads";
    private static final String PRIMARY_VOLUME_NAME = "primary";
    private static final String HOME_VOLUME_NAME = "home";
    private static boolean m_initialized = false;

    private Util() {
    }

    public static void init() {
        if (m_initialized) {
            return;
        }

        // workaround https://github.com/golang/go/issues/70508
        System.loadLibrary("androidsignalhandler");
        initSigsysHandler();

        m_initialized = true;
    }

    public static String getAbsolutePathFromStorageAccessFrameworkUri(Context context, final Uri uri) {
        try {
            return getAbsolutePathFromUri(context, DocumentsContract.getTreeDocumentId(uri));
        } catch (IllegalArgumentException e) {
            return getAbsolutePathFromUri(context, DocumentsContract.getDocumentId(uri));
        }
    }

    private static String removeTrailingFileSeparator(String path) {
        return path.endsWith(File.separator) ? path.substring(0, path.length() - 1) : path;
    }

    private static String combinePath(String path1, String path2) {
        return path2.startsWith(File.separator) ? path1 + path2 : path1 + File.separator + path2;
    }

    private static String getAbsolutePathFromUri(Context context, final String documentId) {
        // determine the volumeId which is the scheme of the documentId URI
        final int colon = documentId.indexOf(':');
        final String volumeId = colon >= 0 ? documentId.substring(0, colon) : documentId;

        // determine path of volumeId (e.g. "home", "documents" or "primary") and append the rest of the document path
        final String volumePath = removeTrailingFileSeparator(getVolumePath(volumeId, context));
        final String documentPath = removeTrailingFileSeparator(colon >= 0 ? documentId.substring(colon + 1) : "");
        return documentPath.isEmpty() ? volumePath : combinePath(volumePath, documentPath);
    }

    private static String getVolumePath(final String volumeId, Context context) {
        try {
            if (HOME_VOLUME_NAME.equals(volumeId)) {
                return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS).getAbsolutePath();
            }
            if (DOWNLOADS_VOLUME_NAME.equals(volumeId)) {
                return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
            }
            StorageManager mStorageManager = (StorageManager) context.getSystemService(Context.STORAGE_SERVICE);
            Class<?> storageVolumeClazz = Class.forName("android.os.storage.StorageVolume");
            Method getVolumeList = mStorageManager.getClass().getMethod("getVolumeList");
            Method getUuid = storageVolumeClazz.getMethod("getUuid");
            Method getPath = storageVolumeClazz.getMethod("getPath");
            Method isPrimary = storageVolumeClazz.getMethod("isPrimary");
            Object result = getVolumeList.invoke(mStorageManager);

            final int length = Array.getLength(result);
            for (int i = 0; i < length; i++) {
                Object storageVolumeElement = Array.get(result, i);
                String uuid = (String) getUuid.invoke(storageVolumeElement);
                Boolean primary = (Boolean) isPrimary.invoke(storageVolumeElement);
                Boolean isPrimaryVolume = (primary && PRIMARY_VOLUME_NAME.equals(volumeId));
                Boolean isExternalVolume = ((uuid != null) && uuid.equals(volumeId));
                Log.d(TAG, "Found volume with uuid='" + uuid +
                    "', volumeId='" + volumeId +
                    "', primary=" + primary +
                    ", isPrimaryVolume=" + isPrimaryVolume +
                    ", isExternalVolume=" + isExternalVolume
                );
                if (isPrimaryVolume || isExternalVolume) {
                    return (String) getPath.invoke(storageVolumeElement);
                }
            }
        } catch (Exception e) {
            Log.d(TAG, "Unable to determine volume path for: " + volumeId, e);
        }
        if (PRIMARY_VOLUME_NAME.equals(volumeId)) {
            return Environment.getExternalStorageDirectory().getAbsolutePath();
        }
        if (!volumeId.startsWith("/storage/")) {
            return "/storage/" + volumeId;
        }
        return volumeId;
    }

    @RequiresApi(api = Build.VERSION_CODES.M)
    public static String getGatewayIPv4(final Context context) {
        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        Network activeNetwork = cm.getActiveNetwork();
        if (activeNetwork == null) return null;

        LinkProperties props = cm.getLinkProperties(activeNetwork);
        if (props == null) return null;

        for (RouteInfo route : props.getRoutes()) {
            InetAddress gateway = route.getGateway();
            if (route.isDefaultRoute() && gateway instanceof Inet4Address) {
                return gateway.getHostAddress();
            }
        }
        return null;
    }

    /*
     * Converts a raw file path into a Storage Access Framework Document ID,
     * including support for removable SD cards and USB OTG drives.
     */
    public static String getDocumentIdFromPath(String fullPath) {
        if (fullPath == null || fullPath.isEmpty()) {
            return null;
        }

        // remove trailing slashes for consistent parsing
        if (fullPath.endsWith("/") && fullPath.length() > 1) {
            fullPath = fullPath.substring(0, fullPath.length() - 1);
        }

        final String INTERNAL_STORAGE_PATH = "/storage/emulated/0";
        final String SDCARD_PATH = "/sdcard";
        final String STORAGE_PREFIX = "/storage/";
        final String PRIMARY_AUTHORITY_PREFIX = "primary:";

        // handle internal storage
        if (fullPath.equals(INTERNAL_STORAGE_PATH) || fullPath.equals(SDCARD_PATH)) {
            return PRIMARY_AUTHORITY_PREFIX; // root of internal storage
        } else if (fullPath.startsWith(INTERNAL_STORAGE_PATH + "/")) {
            String relativePath = fullPath.substring(INTERNAL_STORAGE_PATH.length() + 1);
            return PRIMARY_AUTHORITY_PREFIX + relativePath;
        } else if (fullPath.startsWith(SDCARD_PATH + "/")) {
            String relativePath = fullPath.substring(SDCARD_PATH.length() + 1);
            return PRIMARY_AUTHORITY_PREFIX + relativePath;
        }

        // handle removable SD cards and USB drives
        else if (fullPath.startsWith(STORAGE_PREFIX)) {
            // path looks something like: /storage/1A2B-3C4D/books/sync
            String withoutStorage = fullPath.substring(STORAGE_PREFIX.length()); // "1A2B-3C4D/books/sync"
            int firstSlashIndex = withoutStorage.indexOf('/');
            if (firstSlashIndex != -1) {
                // extract the UUID and the relative path
                String uuid = withoutStorage.substring(0, firstSlashIndex); // "1A2B-3C4D"
                String relativePath = withoutStorage.substring(firstSlashIndex + 1); // "books/sync"
                return uuid + ":" + relativePath;
            } else {
                // it is the absolute root of the SD card, e.g. /storage/1A2B-3C4D
                return withoutStorage + ":";
            }
        }

        // path does not match known external storage patterns
        return null;
    }

    private static native void initSigsysHandler();
}
