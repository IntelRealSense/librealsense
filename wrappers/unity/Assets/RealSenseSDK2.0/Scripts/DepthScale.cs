using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System.Linq;
using System;

public class DepthScale : MonoBehaviour {

    public float minDistanceInMeters = 0.5f;
    public float maxDistanceInMeters = 2.0f;

    public Material bGSegMat;
    // Use this for initialization
    void Start() {
        bGSegMat.SetFloat("_MinRange", minDistanceInMeters);
        bGSegMat.SetFloat("_MaxRange", maxDistanceInMeters);
        if (RealSenseDevice.Instance.ActiveProfile != null)
            OnStartStreaming(RealSenseDevice.Instance.ActiveProfile);
        else
            RealSenseDevice.Instance.OnStart += OnStartStreaming;
    }

    private void OnStartStreaming(PipelineProfile profile)
    {
        var sensor = profile.Device.Sensors.First(s =>
        {
            if (!s.Options[Option.DepthUnits].Supported)
                return false;
            return s.Options[Option.DepthUnits].Value > 0;
        });
        if (sensor != null)
        {
            var depthUnits = sensor.Options[Option.DepthUnits].Value;
            bGSegMat.SetFloat("_DepthScale", depthUnits);
            Debug.Log("Depth Scale for BGSeg : " + depthUnits.ToString());
        }
        else
        {
            Debug.Log("No depth sensor found");
        }
    }

    // Update is called once per frame
    void Update () {
		
	}
}
