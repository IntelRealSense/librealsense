using Intel.RealSense.Devices;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Context : IDisposable
    {
        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        public DeviceList Devices => QueryDevices();

        public delegate void OnDevicesChangedDelegate(DeviceList removed, DeviceList added);
        public event OnDevicesChangedDelegate OnDevicesChanged;

        internal HandleRef Instance;

        // Keeps the delegate alive, if we were to assign onDevicesChanged directly, there'll be 
        // no managed reference it, it will be collected and cause a native exception.
        private readonly rs2_devices_changed_callback onDevicesChangedCallback;

        public readonly int apiVersion;
        public string Version
        {
            get
            {
                if (apiVersion / 10000 == 0)
                    return apiVersion.ToString();

                return (apiVersion / 10000) + "." + (apiVersion % 10000) / 100 + "." + (apiVersion % 100);
            }
        }

        /// <summary>
        /// default librealsense context class
        /// </summary>
        public Context()
        {
            apiVersion = NativeMethods.rs2_get_api_version(out var error);
            Instance = new HandleRef(this, NativeMethods.rs2_create_context(apiVersion, out error));

            onDevicesChangedCallback = new rs2_devices_changed_callback(InvokeDevicesChanged);
            NativeMethods.rs2_set_devices_changed_callback(Instance.Handle, onDevicesChangedCallback, IntPtr.Zero, out error);
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <returns></returns>
        public DeviceList QueryDevices(bool include_platform_camera = false)
        {
            var ptr = NativeMethods.rs2_query_devices_ex(Instance.Handle,
                include_platform_camera ? 0xff : 0xfe, out var error);

            return new DeviceList(ptr);
        }

        private void InvokeDevicesChanged(IntPtr removedList, IntPtr addedList, IntPtr userData)
        {
            using (var removed = new DeviceList(removedList))
            using (var added = new DeviceList(addedList))
                OnDevicesChanged?.Invoke(removed, added);
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                    OnDevicesChanged = null;
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                if (Instance.Handle != IntPtr.Zero)
                {
                    NativeMethods.rs2_delete_context(Instance.Handle);
                    Instance = new HandleRef(this, IntPtr.Zero);
                }

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Context()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion
    }


}
