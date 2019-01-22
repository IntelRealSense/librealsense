using Intel.RealSense.Sensors;
using Intel.RealSense.Types;
using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Devices
{
    public class Device : IDisposable
    {
        public CameraInfos Info
        {
            get
            {
                if (cameraInfo == null)
                    cameraInfo = new CameraInfos(Instance);

                return cameraInfo;
            }
        }
        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        public SensorList Sensors => QuerySensors();

        internal IntPtr Instance;

        private CameraInfos cameraInfo;

        internal Device(IntPtr dev)
        {
            //if (dev == IntPtr.Zero)
            //    throw new ArgumentNullException();
            Instance = dev;
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <returns></returns>
        public SensorList QuerySensors()
        {
            var ptr = NativeMethods.rs2_query_sensors(Instance, out var error);
            return new SensorList(ptr);
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    cameraInfo = null;
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                NativeMethods.rs2_delete_device(Instance);
                Instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        ~Device()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        #endregion

        public class CameraInfos
        {
            public string this[CameraInfo info]
            {
                get
                {
                    if (NativeMethods.rs2_supports_device_info(device, info, out var err) > 0)
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_device_info(device, info, out err));

                    return null;
                }
            }

            private readonly IntPtr device;

            public CameraInfos(IntPtr device)
            {
                this.device = device;
            }

        }
    }
}
