package io.github.martchus.syncthingtray;

import android.os.StrictMode;

import org.qtproject.qt.android.bindings.QtApplication;

public class Application extends QtApplication {

    @Override
    public void onCreate() {
        super.onCreate();

        // set VM policy to avoid crash when sending folder URI to file manager
        StrictMode.VmPolicy vmPolicy = new StrictMode.VmPolicy.Builder()
            .detectAll()
            .penaltyLog()
            .build();
        StrictMode.setVmPolicy(vmPolicy);
    }
}
