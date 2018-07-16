using UnityEngine;
using Intel.RealSense;

public class RealSenseStreamAvailability : MonoBehaviour {

    public bool _disableIfResolvable = false;
    /// <summary>
    /// User configuration
    /// </summary>
    public RealSenseConfiguration DeviceConfiguration = new RealSenseConfiguration
    {
        mode = RealSenseConfiguration.Mode.Live,
        RequestedSerialNumber = string.Empty
    };

    // Use this for initialization
    void Start () {
        RealSenseDeviceListener.Instance.OnDeviceAvailable += Resolve;
        RealSenseDeviceListener.Instance.OnDeviceUnavailable += Resolve;
        Resolve();
    }

    void Resolve()
    {
        var pipeline = new Pipeline();        
        var config = DeviceConfiguration.ToPipelineConfig();
        var isResolvebale = config.CanResolve(pipeline);
        gameObject.SetActive(isResolvebale ^ _disableIfResolvable);
    }
}
