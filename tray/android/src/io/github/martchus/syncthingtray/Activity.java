package io.github.martchus.syncthingtray;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.provider.DocumentsContract;
import android.net.Uri;
import android.view.HapticFeedbackConstants;
import android.view.View;
import android.widget.Toast;

import androidx.core.content.FileProvider;

import java.io.File;

import org.qtproject.qt.android.bindings.QtActivity;

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
        File file = new File(path);
        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri fileUri = FileProvider.getUriForFile(this, getApplicationContext().getPackageName() + ".qtprovider", file);
        intent.addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        if (file.isDirectory()) {
            //intent.setDataAndType(fileUri, DocumentsContract.Document.MIME_TYPE_DIR);
            intent.setDataAndType(fileUri, "resource/folder");
            intent.putExtra("org.openintents.extra.ABSOLUTE_PATH", fileUri);
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
}
