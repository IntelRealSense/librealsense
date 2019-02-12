using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Device : IDisposable
    {
        public IntPtr m_instance;

        internal Device(IntPtr dev)
        {
            //if (dev == IntPtr.Zero)
            //    throw new ArgumentNullException();
            m_instance = dev;
        }

        public class CameraInfos
        {
            readonly IntPtr m_device;
            public CameraInfos(IntPtr device) { m_device = device; }

            public string this[CameraInfo info]
            {
                get
                {
                    object err;
                    if (NativeMethods.rs2_supports_device_info(m_device, info, out err) > 0)
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_device_info(m_device, info, out err));
                    return null;
                }
            }
        }

        CameraInfos m_info;

        public CameraInfos Info
        {
            get
            {
                if (m_info == null)
                    m_info = new CameraInfos(m_instance);
                return m_info;
            }
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        /// <returns></returns>
        public SensorList QuerySensors()
        {
            object error;
            var ptr = NativeMethods.rs2_query_sensors(m_instance, out error);
            return new SensorList(ptr);
        }

        /// <summary>
        /// create a static snapshot of all connected devices at the time of the call
        /// </summary>
        public SensorList Sensors
        {
            get
            {
                return QuerySensors();
            }
        }
        
        public void HardwareReset()
        {
                object error;
                NativeMethods.rs2_hardware_reset(m_instance, out error);
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
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                NativeMethods.rs2_delete_device(m_instance);

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Device()
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
