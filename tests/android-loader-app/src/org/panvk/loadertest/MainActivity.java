package org.panvk.loadertest;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.TextView;

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
        getWindow().addFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
            | WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
            | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
            | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        setContentView(R.layout.activity_main);
        statusText = findViewById(R.id.status_text);
        surfaceView = findViewById(R.id.surface_view);
        surfaceView.getHolder().addCallback(this);
        Log.i(TAG, "PID_SELF=" + android.os.Process.myPid() + " MainActivity.onCreate");
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "PID_SELF=" + android.os.Process.myPid() + " surfaceCreated valid=" + holder.getSurface().isValid());
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.i(TAG, "PID_SELF=" + android.os.Process.myPid()
            + " surfaceChanged " + width + "x" + height + " valid=" + holder.getSurface().isValid());
        if (testStarted) return;
        if (width <= 0 || height <= 0) return;
        if (!holder.getSurface().isValid()) return;
        testStarted = true;
        statusText.setText("Surface ready " + width + "x" + height + ". Starting tests...");
        final Surface surface = holder.getSurface();
        final String appDir = getFilesDir().getAbsolutePath();
        new Thread(() -> {
            String zipPath = "/data/local/tmp/PanVK-Kbase-Android-g615-v11-csf-v0.1.0-beta.1-5a07217f.adpkg.zip";
            Log.i(TAG, "PID_SELF=" + android.os.Process.myPid() + " Starting runPanvkTest with zip=" + zipPath);
            final String result = runPanvkTest(surface, appDir, zipPath);
            Log.i(TAG, "PID_SELF=" + android.os.Process.myPid() + " runPanvkTest completed: " + result);
            runOnUiThread(() -> {
                statusText.setText(result);
                finish();
            });
        }).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "PID_SELF=" + android.os.Process.myPid() + " surfaceDestroyed");
    }
}
