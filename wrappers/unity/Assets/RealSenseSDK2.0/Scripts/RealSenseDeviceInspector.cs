using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;

public class RealSenseDeviceInspector : MonoBehaviour
{
	void Start ()
	{
		if (RealSenseDevice.Instance.ActiveProfile != null) {
			onStartStreaming (RealSenseDevice.Instance.ActiveProfile);
		} else {
			RealSenseDevice.Instance.OnStart += onStartStreaming;
		}
	}

	private void onStartStreaming (PipelineProfile profile)
	{
		device = profile.Device;
		streams = profile.Streams;
		sensors.Clear ();
		foreach (var s in profile.Device.Sensors) {
			sensors.Add (s.Info [CameraInfo.Name], s);
			sensorsOptions.AddRange (s.Options);
		}
		streaming = true;
	}

	public bool streaming;
	public Device device;
	public StreamProfileList streams;
	public readonly Dictionary<string, Sensor> sensors = new Dictionary<string, Sensor> ();
	public List<Sensor.CameraOption> sensorsOptions = new List<Sensor.CameraOption>();
}