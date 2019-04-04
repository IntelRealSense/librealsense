using Intel.RealSense;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using UnityEngine;

public class RsDeviceListener : MonoBehaviour
{
    public static RsDeviceListener Instance { get; private set; }
    private Context ctx;
    private Pipeline pipeline;
    readonly List<Device> m_added = new List<Device>();
    readonly List<Device> m_removed = new List<Device>();
    readonly AutoResetEvent e = new AutoResetEvent(false);

    void Awake()
    {
        if (Instance != null && Instance != this)
            throw new Exception(string.Format("{0} singleton already instanced", this.GetType()));
        Instance = this;
    }

    IEnumerator Start()
    {
        ctx = new Context();
        pipeline = new Pipeline(ctx);

        ctx.OnDevicesChanged += OnDevicesChanged;

        yield return null;

        e.Set();
    }

    void Update()
    {
        if (e.WaitOne(0))
        {
            var avail = FindObjectsOfType<RsStreamAvailability>();
            int tasks = avail.Count();
            AutoResetEvent done = new AutoResetEvent(false);
            Dictionary<RsStreamAvailability, bool> resolvables = new Dictionary<RsStreamAvailability, bool>(tasks);
            foreach (var a in avail)
            {
                ThreadPool.QueueUserWorkItem(state =>
                {
                    using (var config = a.DeviceConfiguration.ToPipelineConfig())
                    {
                        resolvables[a] = config.CanResolve(pipeline);
                    }

                    if (0 == Interlocked.Decrement(ref tasks))
                        done.Set();
                });
            }

            done.WaitOne();

            foreach (var kv in resolvables)
            {
                if (kv.Value)
                    kv.Key.OnDeviceAvailable.Invoke();
                else
                    kv.Key.OnDeviceUnAvailable.Invoke();
            }

        }
    }


    private void OnDevicesChanged(DeviceList removed, DeviceList added)
    {
        try
        {
            foreach (var d in added)
                Debug.LogFormat("{0} {1}", d.Info[CameraInfo.Name], d.Info[CameraInfo.SerialNumber]);

            m_removed.Clear();

            foreach (var d in m_added)
            {
                if (removed.Contains(d))
                    m_removed.Add(d);
            }

            m_added.Clear();
            m_added.AddRange(added);

            e.Set();

        }
        catch (Exception e)
        {
            Debug.LogException(e);
        }
    }

    void OnDestroy()
    {
        foreach (var d in m_added)
            d.Dispose();
        m_added.Clear();
        m_removed.Clear();

        if (pipeline != null)
        {
            pipeline.Dispose();
            pipeline = null;
        }

        if (ctx != null)
        {
            ctx.OnDevicesChanged -= OnDevicesChanged;
            ctx.Dispose();
            ctx = null;
        }
    }
}
