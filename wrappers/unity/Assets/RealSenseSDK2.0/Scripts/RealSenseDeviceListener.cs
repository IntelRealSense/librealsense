using Intel.RealSense;
using System;
using UnityEngine;

public class RealSenseDeviceListener : MonoBehaviour {

    public static RealSenseDeviceListener Instance { get; private set; }

    /// <summary>
    /// Notifies when a RealSenseDevice is available
    /// </summary>
    public event Action OnDeviceAvailable;

    /// <summary>
    /// Notifies when a RealSenseDevice is available
    /// </summary>
    public event Action OnDeviceUnavailable;

    private bool _isConnected = false;

    void Awake()
    {
        if (Instance != null && Instance != this)
            throw new Exception(string.Format("{0} singleton already instanced", this.GetType()));
        Instance = this;
    }

    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
        using (var ctx = new Context())
        {
            var isConnected = ctx.QueryDevices().Count > 0 ;
            if (isConnected != _isConnected)
            {
                _isConnected = isConnected;
                if (isConnected)
                    OnDeviceAvailable();
                else
                    OnDeviceUnavailable();
            }
        }       
    }
}
