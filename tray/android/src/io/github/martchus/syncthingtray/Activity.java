package io.github.martchus.syncthingtray;

import android.view.HapticFeedbackConstants;
import android.view.View;
import android.widget.Toast;
import org.qtproject.qt.android.bindings.QtActivity;

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

}
