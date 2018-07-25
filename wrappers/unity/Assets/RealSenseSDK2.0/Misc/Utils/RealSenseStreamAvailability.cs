using UnityEngine;
using Intel.RealSense;
using System.Collections;
using System;
using UnityEngine.Events;

public class RealSenseStreamAvailability : MonoBehaviour
{
    public RealSenseConfiguration DeviceConfiguration;

    [Space]
    public UnityEvent OnDeviceAvailable;
    public UnityEvent OnDeviceUnAvailable;
}
