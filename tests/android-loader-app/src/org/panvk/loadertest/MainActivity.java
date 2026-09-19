package org.panvk.loadertest;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "PanVKLoaderApp";
    private TextView statusText;
    private SurfaceView surfaceView;
    private boolean testStarted = false;

    static {
        System.loadLibrary("panvk_loader_test");
    }

    public native String runPanvkTest(Surface surface, String appDir, String zipPath);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        statusText = findViewById(R.id.status_text);
        surfaceView = findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (testStarted) return;
        testStarted = true;
        statusText.setText("Surface created. Starting tests...");
        final Surface surface = holder.getSurface();
        final String appDir = getFilesDir().getAbsolutePath();

        new Thread(() -> {
            String zipPath = "/data/local/tmp/PanVK-Kbase-Android-g615-v11-csf-v0.1.0-beta.1-5a07217f.adpkg.zip";
            Log.i(TAG, "Starting runPanvkTest with zip=" + zipPath);
            final String result = runPanvkTest(surface, appDir, zipPath);
            Log.i(TAG, "runPanvkTest completed: " + result);
            runOnUiThread(() -> statusText.setText(result));
        }).start();
    }

    @Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}
    @Override public void surfaceDestroyed(SurfaceHolder holder) {}
}
