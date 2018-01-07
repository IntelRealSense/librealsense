package com.example.realsense_app;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;
import java.util.concurrent.atomic.AtomicBoolean;


public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    static String className;
    static SurfaceView mSurfaceView;
    static SurfaceHolder mSurfaceHolder;
    static Paint paint = new Paint();
    static AtomicBoolean surfaceExists = new AtomicBoolean(false);

    public static void onFrame(Object frameBuff, int width, int height)
    {
        if (!surfaceExists.get())
            return;

        byte[] data = (byte[])frameBuff;
        int intByteCount = data.length;
        int[] intColors = new int[intByteCount / 3];
        final int intAlpha = 255;
        if ((intByteCount / 3) != (width * height)) {
            throw new ArrayStoreException();
        }

        for (int intIndex = 0; intIndex < intByteCount - 2; intIndex = intIndex + 3) {
            intColors[intIndex / 3] = (intAlpha << 24) | (data[intIndex] << 16) | (data[intIndex + 1] << 8) | data[intIndex + 2];
        }

        Bitmap bitmap = Bitmap.createBitmap(intColors, width, height, Bitmap.Config.ARGB_8888);
        bitmap = Bitmap.createScaledBitmap(bitmap, mSurfaceView.getWidth(),
                mSurfaceView.getHeight(),
                true);
        Canvas rCanvas = mSurfaceHolder.lockCanvas();

        rCanvas.drawColor(Color.TRANSPARENT, PorterDuff.Mode.CLEAR);

        // draw the bitmap
        rCanvas.drawBitmap(bitmap, 0, 0, paint);
        mSurfaceHolder.unlockCanvasAndPost(rCanvas);
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        try{
            super.onCreate(savedInstanceState);
            setContentView(R.layout.activity_main);

            className = getClass().getName().replace('.', '/');

            paint.setColor(0xFFFFFFFF);
            paint.setStyle(Paint.Style.STROKE);

            mSurfaceView = (SurfaceView) this.findViewById(R.id.camera_surface_view);
            mSurfaceHolder = mSurfaceView.getHolder();
            mSurfaceHolder.addCallback(this);

            startStreaming(className);
        }
        catch (Exception ex)
        {
            Toast.makeText(getBaseContext(), ex.getMessage(),
                    Toast.LENGTH_LONG).show();
        }
    }

    @Override
    protected void onDestroy() {
        try {
            mSurfaceHolder.removeCallback(this);
            stopStreaming();
            super.onDestroy();
        }
        catch (Exception ex)
        {
            Toast.makeText(getBaseContext(), ex.getMessage(),
                    Toast.LENGTH_LONG).show();
        }
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void startStreaming(String className);
    public native void stopStreaming();

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        surfaceExists.set(true);
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        surfaceExists.set(false);
    }
}
