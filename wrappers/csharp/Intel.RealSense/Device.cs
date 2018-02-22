using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class DeviceList : IDisposable, IEnumerable<Device>
    {
        IntPtr m_instance;

        public DeviceList(IntPtr ptr)
        {
            m_instance = ptr;
        }


        public IEnumerator<Device> GetEnumerator()
        {
            object error;

            int deviceCount = NativeMethods.rs2_get_device_count(m_instance, out error);
            for (int i = 0; i < deviceCount; i++)
            {
                var ptr = NativeMethods.rs2_create_device(m_instance, i, out error);
                yield return new Device(ptr);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public int Count
        {
            get
            {
                object error;
                int deviceCount = NativeMethods.rs2_get_device_count(m_instance, out error);
                return deviceCount;
            }
        }

        public Device this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_create_device(m_instance, index, out error);
                return new Device(ptr);
            }
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
                NativeMethods.rs2_delete_device_list(m_instance);
                m_instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~DeviceList()
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

    public class Device
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
            IntPtr m_device;
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
    }

    public class AdvancedDevice : Device
    {
        internal AdvancedDevice(IntPtr dev) : base(dev)
        {

        }

        public static AdvancedDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.m_instance, Extension.AdvancedMode, out error) == 0)
            {
                throw new ArgumentException("Device does not support AdvancedMode");
            }

            return new AdvancedDevice(dev.m_instance);
        }

        public bool AdvancedModeEnabled
        {
            get
            {
                int enabled = 0;
                object error;
                NativeMethods.rs2_is_enabled(m_instance, out enabled, out error);

                return enabled == 1 ? true : false;
            }
            set
            {
                object error;
                NativeMethods.rs2_toggle_advanced_mode(m_instance, value ? 1 : 0, out error);
            }
        }

        public string JsonConfiguration
        {
            get
            {
                object error;
                IntPtr buffer = NativeMethods.rs2_serialize_json(m_instance, out error);
                int size = NativeMethods.rs2_get_raw_data_size(buffer, out error);
                IntPtr data = NativeMethods.rs2_get_raw_data(buffer, out error);

                return Marshal.PtrToStringAnsi(data, size);
            }
            set
            {
                object error;
                NativeMethods.rs2_load_json(m_instance, value, (uint)value.ToCharArray().Length, out error);
            }
        }
    }

}
