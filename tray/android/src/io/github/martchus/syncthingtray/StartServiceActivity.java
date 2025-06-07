import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Build;

import io.github.martchus.syncthingtray.SyncthingService;

public class StartServiceActivity extends Activity {
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = new Intent(this, SyncthingService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
        finish();
    }
}