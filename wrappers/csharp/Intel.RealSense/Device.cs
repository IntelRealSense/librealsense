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

        public bool Contains(Device device)
        {
            object error;
            return System.Convert.ToBoolean(NativeMethods.rs2_device_list_contains(m_instance, device.m_instance, out error));
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

    public class PlaybackDevice : Device
    {
        internal PlaybackDevice(IntPtr dev) : base(dev)
        {

        }


        protected override void Dispose(bool disposing)
        {
            // Intentionally empty, does not own the native device, only wraps it.
        }

        public static PlaybackDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.m_instance, Extension.Playback, out error) == 0)
            {
                throw new ArgumentException("Device does not support Playback");
            }

            return new PlaybackDevice(dev.m_instance);
        }

        public void Pause()
        {
            object error;
            NativeMethods.rs2_playback_device_pause(m_instance, out error);
        }

        public void Resume()
        {
            object error;
            NativeMethods.rs2_playback_device_resume(m_instance, out error);
        }

        public PlaybackStatus Status
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_get_current_status(m_instance, out error);
            }
        }

        public ulong Duration
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_duration(m_instance, out error);
            }
        }

        public ulong Position
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_get_position(m_instance, out error);
            }
            set
            {
                object error;
                NativeMethods.rs2_playback_seek(m_instance, (long)value, out error);
            }
        }

        public void Seek(long time)
        {
            object error;
            NativeMethods.rs2_playback_seek(m_instance, time, out error);
        }

        public bool Realtime
        {
            get
            {
                object error;
                return NativeMethods.rs2_playback_device_is_real_time(m_instance, out error) != 0;
            }
            set
            {
                object error;
                NativeMethods.rs2_playback_device_set_real_time(m_instance, value ? 1 : 0, out error);
            }
        }

        public float Speed
        {
            set
            {
                object error;
                NativeMethods.rs2_playback_device_set_playback_speed(m_instance, value, out error);
            }
        }
    }


    public class RecordDevice : Device
    {
        readonly IntPtr m_dev;

        public RecordDevice(Device dev, string file) : base(IntPtr.Zero)
        {
            m_dev = dev.m_instance;
            object error;
            m_instance = NativeMethods.rs2_create_record_device(m_dev, file, out error);
        }

        public void Pause()
        {
            object error;
            NativeMethods.rs2_record_device_pause(m_instance, out error);
        }

        public void Resume()
        {
            object error;
            NativeMethods.rs2_record_device_resume(m_instance, out error);
        }

        public string Filename
        {
            get
            {
                object error;
                var p = NativeMethods.rs2_record_device_filename(m_instance, out error);
                return Marshal.PtrToStringAnsi(p);
            }
        }
    }
}
