/*
 * This code is based on com.nutomic.syncthingandroid.util from
 * https://github.com/Catfriend1/syncthing-android/blob/main/app/src/main/java/com/nutomic/syncthingandroid/util/FileUtils.java
 */

package io.github.martchus.syncthingtray;

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
}
