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
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));
        Assert.NotNull(go);
        Assert.NotNull(go.GetComponent<RsDevice>());

        var rs = RsDevice.Instance;
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

        rs.OnNewSample += sample =>
        {
            Debug.Log(sample);
        };

        GameObject.Destroy(go);
        yield return null;
        Assert.That(go == null);
        Assert.IsNull(RsDevice.Instance);
    }

    [UnityTest]
    [Category("Live")]
    public IEnumerator TestLiveDepthTexture()
    {
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));

        var depth_go = new GameObject("Depth");
        var depth = depth_go.AddComponent<RsStreamTextureRenderer>();

        depth._stream = Stream.Depth;
        depth._format = Format.Z16;

        depth.textureBinding = new RsStreamTextureRenderer.TextureEvent();
        Assert.IsNotNull(depth.textureBinding);

        Texture depth_tex = null;
        depth.textureBinding.AddListener(t => depth_tex = t);

        yield return new WaitUntil(() => RsDevice.Instance.Streaming);
        Assert.IsNotNull(depth_tex);

        using (var depth_profile = RsDevice.Instance.ActiveProfile.GetStream(Stream.Depth) as VideoStreamProfile)
        {
            Assert.AreEqual(depth_tex.width, depth_profile.Width);
            Assert.AreEqual(depth_tex.height, depth_profile.Height);
        }

        GameObject.Destroy(depth_go);
        GameObject.Destroy(go);
    }


    [UnityTest]
    [Category("Live")]
    public IEnumerator TestDisabledNoStart()
    {
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));
        var rs = RsDevice.Instance;

        rs.OnStop += Assert.Fail;
        rs.OnStart += delegate
        {
            Assert.Fail();
        };

        go.SetActive(false);

        GameObject.Destroy(go);
        yield return null;
        Assert.That(go == null);
        Assert.IsNull(RsDevice.Instance);
    }

    [UnityTest]
    [Category("Live")]
    public IEnumerator TestDisableStop()
    {
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));
        var rs = RsDevice.Instance;

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
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));
        var rs = RsDevice.Instance;

        var depth = rs.ActiveProfile.GetStream(Stream.Depth);
        Assert.IsNotNull(depth);

        var color = rs.ActiveProfile.GetStream(Stream.Color);
        Assert.IsNotNull(color);

        var ex = depth.GetExtrinsicsTo(color);
        Assert.True(Array.TrueForAll(ex.translation, x => x != 0));
        Assert.True(Array.TrueForAll(ex.rotation, x => x != 0));

        GameObject.Destroy(go);
    }

    [UnityTest]
    [Category("Live"), Category("Record")]
    public IEnumerator TestRecord()
    {
        var go = new GameObject("RealSenseDevice", typeof(RsDevice));
        var rs = RsDevice.Instance;

        go.SetActive(false);

        rs.processMode = RsDevice.ProcessMode.UnityThread;
        rs.DeviceConfiguration.mode = RsConfiguration.Mode.Record;
        var path = "D:/test.bag";
        rs.DeviceConfiguration.RecordPath = path;

        go.SetActive(true);

        yield return new WaitForSeconds(2f);
        GameObject.Destroy(go);
        yield return null;

        Assert.That(go == null);

        rs = null;

        // GC.Collect();
        // GC.WaitForPendingFinalizers();

        var fi = new System.IO.FileInfo(path);
        fi.Open(System.IO.FileMode.Open, System.IO.FileAccess.Read);
        Assert.True(fi.Exists);
        Assert.Greater(fi.Length, 0);
    }


    [UnityTest]
    [Category("Playback")]
    public IEnumerator TestPlayback()
    {
        var go = new GameObject("RealSenseDevice");
        go.SetActive(false);

        var rs = go.AddComponent<RsDevice>();

        Debug.Log(JsonUtility.ToJson(rs.DeviceConfiguration, true));

        var path = "D:/test.bag";
        var fi = new System.IO.FileInfo(path);
        Assert.True(fi.Exists);
        rs.DeviceConfiguration.PlaybackFile = path;
        rs.DeviceConfiguration.mode = RsConfiguration.Mode.Playback;
        rs.DeviceConfiguration.Profiles = null;

        Debug.Log(JsonUtility.ToJson(rs.DeviceConfiguration, true));

        yield return null;

        go.SetActive(true);

        Debug.Log(rs.ActiveProfile.Streams.Count);

        yield return new WaitForSeconds(2f);
        GameObject.Destroy(go);
        yield return null;
    }
}
