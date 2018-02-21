using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System.Linq;

public class DepthScale : MonoBehaviour {

    public float minDistanceInMeters = 0.5f;
    public float maxDistanceInMeters = 2.0f;

    public Material bGSegMat;
    // Use this for initialization
    void Start () {
        bGSegMat.SetFloat("_MinRange", minDistanceInMeters);
        bGSegMat.SetFloat("_MaxRange", maxDistanceInMeters);
        Debug.Log("Depth Scale!");
        var sensor = RealSenseDevice.Instance.ActiveProfile.Device.Sensors.First(s => s.DepthScale != 0);
        if (sensor != null)
        {
            bGSegMat.SetFloat("_DepthScale", sensor.DepthScale);
            Debug.Log("Depth Scale for BGSeg : " + sensor.DepthScale.ToString());
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
