using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System;
using System.Collections;
using System.Linq;

public class RsDeviceInspector : MonoBehaviour
{
    RsDevice rsdevice;

    public bool streaming;
    public Device device;
    public readonly Dictionary<string, Sensor> sensors = new Dictionary<string, Sensor>();
    public readonly Dictionary<string, List<IOption>> sensorOptions = new Dictionary<string, List<IOption>>();

    void Awake()
    {
        StartCoroutine(WaitForDevice());
    }

    private IEnumerator WaitForDevice()
    {
        while (true)
        {
            yield return new WaitUntil(() => (rsdevice = GetComponent<RsDevice>()) != null);
            // rsdevice.OnStart += onStartStreaming;
            rsdevice.OnStop += onStopStreaming;

            yield return new WaitUntil(() => rsdevice.Streaming);
            onStartStreaming(rsdevice.ActiveProfile);
            yield return new WaitWhile(() => rsdevice.Streaming);
        }
    }

    private void onStopStreaming()
    {
        streaming = false;

        if (device != null)
        {
            device.Dispose();
            device = null;
        }

        foreach (var s in sensors)
        {
            var sensor = s.Value;
            if (sensor != null)
                sensor.Dispose();
        }
        sensors.Clear();
        sensorOptions.Clear();
    }

    private void onStartStreaming(PipelineProfile profile)
    {
        device = profile.Device;
        foreach (var s in device.Sensors)
        {
            var sensorName = s.Info[CameraInfo.Name];
            sensors.Add(sensorName, s);
            sensorOptions.Add(sensorName, s.Options.ToList());
        }
        streaming = true;
    }
}