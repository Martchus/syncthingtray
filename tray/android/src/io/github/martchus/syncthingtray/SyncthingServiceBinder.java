package io.github.martchus.syncthingtray;

import android.os.Binder;

public class SyncthingServiceBinder extends Binder {
    private final SyncthingService m_service;

    public SyncthingServiceBinder(SyncthingService service) {
        m_service = service;
    }

    public SyncthingService getService() {
        return m_service;
    }
}
