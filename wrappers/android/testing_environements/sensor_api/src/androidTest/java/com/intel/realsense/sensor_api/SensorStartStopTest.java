package com.intel.realsense.sensor_api;

import android.Manifest;
import android.util.Log;

import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;
import androidx.test.rule.ActivityTestRule;
import androidx.test.rule.GrantPermissionRule;

import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamType;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.RuleChain;
import org.junit.runner.RunWith;

import java.util.List;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class SensorStartStopTest {

    public ActivityTestRule<MainActivity> mActivityRule =
            new ActivityTestRule(MainActivity.class, true, true);
    private MainActivity mActivity;


    @Rule
    public RuleChain chain = RuleChain.outerRule(mActivityRule).
            around(GrantPermissionRule.grant(Manifest.permission.CAMERA));

    @Before
    public void adjustProfiles() {
        mActivity = mActivityRule.getActivity();
        mActivity.changeProfile(StreamType.DEPTH, -1, 1280, 720,
                StreamFormat.Z16, 30);
    }

    @Test
    public void testStartStopOnce() {
        mActivity = mActivityRule.getActivity();
        mActivity.start();
        try{Thread.sleep(5000);} catch(Exception e){}
        mActivity.stop();
        try{Thread.sleep(2000);} catch(Exception e){}
    }

    @Test
    public void testStartTopMany() {
        mActivity = mActivityRule.getActivity();
        int iterations = 200;
        while (--iterations > 0) {
            mActivity.start();
            try{Thread.sleep(5000);} catch(Exception e){}
            mActivity.stop();
            try{Thread.sleep(2000);} catch(Exception e){}
        }
    }

    @Test
    public void testDeviceNotNull() {
        ActivityScenario<MainActivity> activityScenario =
                ActivityScenario.launch(MainActivity.class);
        assert(activityScenario != null);
        activityScenario.onActivity(
                activity -> {
                    //assert(activity.getDevice() != null);
                });
    }
}
