/*
 * This code is based on com.nutomic.syncthingandroid.util from
 * https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java
 */

package io.github.martchus.syncthingtray;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.Uri;
import android.os.Environment;
import android.os.storage.StorageManager;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.File;
import java.lang.IllegalArgumentException;
import java.lang.reflect.Array;
import java.lang.reflect.Method;

public class Util {

    private static final String TAG = "Util";
    private static final String DOWNLOADS_VOLUME_NAME = "downloads";
    private static final String PRIMARY_VOLUME_NAME = "primary";
    private static final String HOME_VOLUME_NAME = "home";

    private Util() {
    }

    public static String getAbsolutePathFromStorageAccessFrameworkUri(Context context, final Uri uri) {
        try {
            return getAbsolutePathFromUri(context, DocumentsContract.getTreeDocumentId(uri));
        } catch (IllegalArgumentException e) {
            return getAbsolutePathFromUri(context, DocumentsContract.getDocumentId(uri));
        }
    }

    private static String getAbsolutePathFromUri(Context context, final String documentId) {
        // determine volumeId, e.g. "home", "documents"
        String volumeId = getVolumeIdFromDocument(documentId);
        if (volumeId == null) {
            return "";
        }

        // handle Uri referring to internal or external storage.
        String volumePath = getVolumePath(volumeId, context);
        if (volumePath == null) {
            return File.separator;
        }
        if (volumePath.endsWith(File.separator)) {
            volumePath = volumePath.substring(0, volumePath.length() - 1);
        }
        String documentPath = getPathFromDocument(documentId);
        if (documentPath.endsWith(File.separator)) {
            documentPath = documentPath.substring(0, documentPath.length() - 1);
        }
        if (documentPath.length() > 0) {
            if (documentPath.startsWith(File.separator)) {
                return volumePath + documentPath;
            } else {
                return volumePath + File.separator + documentPath;
            }
        } else {
            return volumePath;
        }
    }

    @SuppressLint("ObsoleteSdkInt")
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
            Log.w(TAG, "Ran into exception when determining volume path for " + volumeId, e);
        }
        Log.d(TAG, "Unable to determine volume path for " + volumeId);
        if (PRIMARY_VOLUME_NAME.equals(volumeId)) {
            return Environment.getExternalStorageDirectory().getAbsolutePath();
        }
        return "/storage/" + volumeId;
    }

    private static String getVolumeIdFromDocument(final String documentId) {
        final String[] split = documentId.split(":");
        return split.length > 0 ? split[0] : null;
    }

    private static String getPathFromDocument(final String documentId) {
        final String[] split = documentId.split(":");
        return ((split.length >= 2) && (split[1] != null)) ? split[1] : File.separator;
    }
}
