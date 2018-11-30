package com.intel.realsense.libusbhost;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.Toast;

import java.nio.ByteBuffer;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int CAMERA_PERMISSIONS = 1;

    static {
        System.loadLibrary("usbhost");
    }

    final Handler mHandler = new Handler();
    private ByteBuffer buffer;

    private ImageView outputImage;
    private Bitmap bitmap;
    private Button btnStart;
    private static final int DEPTH_HEIGHT = 480;
    private static final int DEPTH_WIDTH = 848;
    private ColorConverter mColorConverter;
    private RealsenseUsbHostManager mUsbHostManager;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mUsbHostManager = new RealsenseUsbHostManager(this);
        setContentView(R.layout.activity_main);
        buffer=ByteBuffer.allocateDirect(DEPTH_HEIGHT*DEPTH_WIDTH*2);
        bitmap = Bitmap.createBitmap(DEPTH_WIDTH,DEPTH_HEIGHT, Bitmap.Config.ARGB_8888);
        mColorConverter=new ColorConverter(this,ConversionType.DEPTH2RGBA);
        outputImage = (ImageView) findViewById(R.id.output);
        btnStart=(Button)findViewById(R.id.btnStart);
        btnStart.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                btnStart.setText(R.string.stream_started);
                btnStart.setEnabled(false);
                startRepeatingTask();
                librsStartStreaming(buffer);
            }
        });
        getCameraPermission();


    }



    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        closeDevice();
    }

    Runnable updateBitmap = new Runnable() {
        @Override
        public void run() {
            try {

                mColorConverter.toBitmap(buffer,bitmap);
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        outputImage.setImageBitmap(bitmap);
                    }
                });
            } finally {
                // 100% guarantee that this always happens, even if
                // your update method throws an exception
                mHandler.postDelayed(updateBitmap, 1000 / 30);
            }
        }
    };
    private void getCameraPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                == PackageManager.PERMISSION_DENIED)
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, CAMERA_PERMISSIONS);
        else
            getUsbPermissions();
    }

    private void getUsbPermissions() {
        if(mUsbHostManager.findDevice()) {
            btnStart.setText(R.string.start_streaming);
            btnStart.setEnabled(true);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == CAMERA_PERMISSIONS) {

            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {

                Toast.makeText(this, "camera permission granted", Toast.LENGTH_LONG).show();
                getUsbPermissions();

            } else {
                Toast.makeText(this, "camera permission denied", Toast.LENGTH_LONG).show();
            }

        }
    }//end onRequestPermissionsResult



    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     *
     * @param l
     */
    public native boolean librsStartStreaming(ByteBuffer l);

    public native boolean librsStopStreaming();

    void startRepeatingTask() {
        updateBitmap.run();
    }
    void stopRepeatingTask() {
        mHandler.removeCallbacks(updateBitmap);
    }
    private void closeDevice() {
        stopRepeatingTask();
        librsStopStreaming();
    }


}
