using UnityEngine;
using UnityEngine.TestTools;
using NUnit.Framework;
using System.Collections;
using System;
using Intel.RealSense;

public class NewPlayModeTest
{
    // A UnityTest behaves like a coroutine in PlayMode
    // and allows you to yield null to skip a frame in EditMode
    [UnityTest]
    [Category("Live")]
    public IEnumerator TestLiveCamera()
    {
        var go = new GameObject("RealSenseDevice", typeof(RealSenseDevice));
        Assert.NotNull(go);
        Assert.NotNull(go.GetComponent<RealSenseDevice>());

        var rs = RealSenseDevice.Instance;
        Assert.NotNull(rs);

        bool started = false;
        rs.OnStart += delegate
        {
            Assert.IsFalse(started);
            started = true;
        };

        Assert.IsFalse(started);
        yield return new WaitUntil(() => started);
        Assert.IsTrue(started);

        rs.onNewSample += sample =>
        {
            Debug.Log(sample);
        };

        GameObject.Destroy(go);
        yield return null;
        Assert.That(go == null);
    }

    [UnityTest]
    [Category("Live")]
    public IEnumerator TestDisabledNoStart()
    {
        var go = new GameObject("RealSenseDevice", typeof(RealSenseDevice));
        var rs = RealSenseDevice.Instance;

        rs.OnStop += Assert.Fail;
        rs.OnStart += delegate
        {
            Assert.Fail();
        };

        go.SetActive(false);

        GameObject.Destroy(go);
        yield return null;
        Assert.That(go == null);
    }

    [UnityTest]
    [Category("Live")]
    public IEnumerator TestDisableStop()
    {
        var go = new GameObject("RealSenseDevice", typeof(RealSenseDevice));
        var rs = RealSenseDevice.Instance;

        bool started = false;
        rs.OnStart += delegate
        {
            Assert.True(started = true);
        };

        yield return new WaitUntil(() => started);

        bool stopped = false;
        rs.OnStop += delegate
        {
            Assert.True(stopped = true);
        };

        go.SetActive(false);

        yield return new WaitUntil(() => stopped);

        GameObject.Destroy(go);
        yield return null;
        Assert.That(go == null);
    }

    [Test]
    [Category("Live")]
    public void TestExtrinsics()
    {
        var go = new GameObject("RealSenseDevice", typeof(RealSenseDevice));
        var rs = RealSenseDevice.Instance;

        var depth = rs.ActiveProfile.GetStream(Stream.Depth);
        Assert.IsNotNull(depth);

        var color = rs.ActiveProfile.GetStream(Stream.Color);
        Assert.IsNotNull(color);

        var ex = depth.GetExtrinsicsTo(color);
        Assert.True(Array.TrueForAll(ex.translation, x => x != 0));
        Assert.True(Array.TrueForAll(ex.rotation, x => x != 0));

        GameObject.Destroy(go);
    }
}
