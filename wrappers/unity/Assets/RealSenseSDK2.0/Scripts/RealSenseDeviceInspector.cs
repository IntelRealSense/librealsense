using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System;
using System.Collections;

public class RealSenseDeviceInspector : MonoBehaviour
{

    public bool streaming;
    public Device device;
    public StreamProfileList streams;
    public readonly Dictionary<string, Sensor> sensors = new Dictionary<string, Sensor>();
    public List<Sensor.CameraOption> sensorsOptions = new List<Sensor.CameraOption>();

    void Awake()
    {
        StartCoroutine(WaitForDevice());
    }

    private IEnumerator WaitForDevice()
    {
        yield return new WaitUntil(() => RealSenseDevice.Instance != null);
        RealSenseDevice.Instance.OnStart += onStartStreaming;
        RealSenseDevice.Instance.OnStop += onStopStreaming;
    }

    private void onStopStreaming()
    {
        streaming = false;
        device = null;
        if (streams != null)
        {
            streams.Dispose();
            streams = null;
        }
        sensors.Clear();
    }

    private void onStartStreaming(PipelineProfile profile)
    {
        device = profile.Device;
        streams = profile.Streams;
        sensors.Clear();
        foreach (var s in profile.Device.Sensors)
        {
            sensors.Add(s.Info[CameraInfo.Name], s);
            sensorsOptions.AddRange(s.Options);
        }
        streaming = true;
    }
}