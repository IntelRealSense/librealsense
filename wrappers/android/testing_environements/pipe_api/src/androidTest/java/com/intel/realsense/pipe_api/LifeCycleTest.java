package com.intel.realsense.pipe_api;

import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.rules.ActivityScenarioRule;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.LargeTest;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
@LargeTest
public class LifeCycleTest {

    public ActivityScenario mScenario;

    @Rule
    public ActivityScenarioRule<MainActivity> activityRule =
            new ActivityScenarioRule<>(MainActivity.class);

    @Before
    public void launchActivity() {
        mScenario = activityRule.getScenario().launch(MainActivity.class);
    }

    @Test
    public void testLifeCycle() {
        ActivityScenario<MainActivity> activityScenario =
                ActivityScenario.launch(MainActivity.class);
        assert(activityScenario != null);
        activityScenario.onActivity(
                activity -> {
                    assert(activity.getLifecycle().getCurrentState() == Lifecycle.State.RESUMED);
                });
        activityScenario.moveToState(Lifecycle.State.STARTED);
        activityScenario.onActivity(
                activity -> {
                    assert(activity.getLifecycle().getCurrentState() == Lifecycle.State.STARTED);
                });
        activityScenario.moveToState(Lifecycle.State.CREATED);
        activityScenario.onActivity(
                activity -> {
                    assert(activity.getLifecycle().getCurrentState() == Lifecycle.State.CREATED);
                });
    }
}
