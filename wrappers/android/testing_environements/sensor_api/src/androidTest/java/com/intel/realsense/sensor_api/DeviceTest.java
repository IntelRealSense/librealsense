package com.intel.realsense.sensor_api;

import android.Manifest;
import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.RsContext;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertTrue;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class DeviceTest {

    private static final String TAG = "librs_sensor_device_test";

    public ActivityTestRule<MainActivity> mActivityRule =
            new ActivityTestRule(MainActivity.class, true, true);
    private MainActivity mActivity;

    @Rule
    public RuleChain chain = RuleChain.outerRule(mActivityRule).
            around(GrantPermissionRule.grant(Manifest.permission.CAMERA));

    @Test
    public void testDeviceFound() {
        mActivity = mActivityRule.getActivity();
        Log.i(TAG, mActivity.getLifecycle().getCurrentState().toString());
        RsContext ctx = new RsContext();
        DeviceList devices = ctx.queryDevices();
        int numOfDevices = devices.getDeviceCount();
        assertTrue (numOfDevices > 0);
    }
}
