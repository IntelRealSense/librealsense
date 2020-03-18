package com.example.realsense_native_example;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.RsContext;

import static java.lang.Thread.sleep;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "librs native example";
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;

    private RsContext mRsContext;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }
    }

    DeviceListener mListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //RsContext.init must be called once in the application's lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(getApplicationContext());

        try {
            // give T265 time to boot
            sleep(1000);
        } catch (InterruptedException e) {
            Log.e(TAG, "onCreate: sleep interrupted");
        }

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(new DeviceListener() {
            @Override
            public void onDeviceAttach() {
                onRSDeviceAttach();
                printMessage();
            }

            @Override
            public void onDeviceDetach() {
                onRSDeviceDetach();
                printMessage();
            }
        });

        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }

        // there appear to be a glitch in DeviceWatcher class and it does not
        // dispatch onDeviceAttach() message after initial TM265 boot
        onRSDeviceAttach();
    }

    private void onRSDeviceDetach() {
        Log.i(TAG, "onRSDeviceDetach()");
        try {
            // you will need to add call to disable your handler here to stop processing the data

            nStopStream();
            Log.i(TAG, "onRSDeviceDetach: stream stopped");
        }
        catch (Exception ex) {
            Log.e(TAG, "onRSDeviceDetach: failed to stop stream");
        }
    }

    private void onRSDeviceAttach() {
        Log.i(TAG, "onRSDeviceAttach()");
        TextView tv = (TextView) findViewById(R.id.sample_text);
        try {
            DeviceList devs= mRsContext.queryDevices();
            int devCount = devs.getDeviceCount();
            tv.setText("onRSDeviceAttach: Connected to: " + devCount);
            Log.i(TAG, "onRSDeviceAttach: " + devCount+ " device(s) available");

            if(devCount>0) {
                Log.i(TAG, "onRSDeviceAttach: start streaming");
                nStreamPoseData();

                // you will need to add call to enable your handler here to start processing the data
            }
        }
        catch (Exception ex) {
            tv.setText("Unable to get list of devices");
            Log.e(TAG, "onRSDeviceAttach: Failed to query list of devices");
        }
    }


    private void printMessage(){
        // Example of a call to native methods
        final String version = nGetLibrealsenseVersionFromJNI();
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final String cameraCountString = "Detected\n" + nGetCamerasCountFromJNI() + " camera(s)\n" + nGetSensorsCountFromJNI() + " sensor(s)";
                TextView tv = (TextView) findViewById(R.id.sample_text);
                tv.setText("This app use librealsense: " + version + "\n" + cameraCountString);
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    private static native String nGetLibrealsenseVersionFromJNI();
    private static native int nGetCamerasCountFromJNI();
    private static native int nGetSensorsCountFromJNI();

    // example calls to native methods to start T265 stream data
    private static synchronized native void nStreamPoseData();
    private static synchronized native void nStopStream();
}

