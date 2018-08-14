using UnityEngine;
using Intel.RealSense;
using System.Collections;
using System;
using UnityEngine.Events;

public class RsStreamAvailability : MonoBehaviour
{
    public RsConfiguration DeviceConfiguration;

    [Space]
    public UnityEvent OnDeviceAvailable;
    public UnityEvent OnDeviceUnAvailable;
}
