﻿using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public interface IOptionsContainer : IEnumerable<IOption>
    {
        IOption this[Option option] { get; }
        string OptionValueDescription(Option option, float value);
    }

    public interface IOptions {
        IOptionsContainer Options { get;  }
    }

    public class SensorList : IDisposable, IEnumerable<Sensor>
    {
        IntPtr m_instance;

        public SensorList(IntPtr ptr)
        {
            m_instance = ptr;
        }


        public IEnumerator<Sensor> GetEnumerator()
        {
            object error;

            int sensorCount = NativeMethods.rs2_get_sensors_count(m_instance, out error);
            for (int i = 0; i < sensorCount; i++)
            {
                var ptr = NativeMethods.rs2_create_sensor(m_instance, i, out error);
                yield return new Sensor(ptr);
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
                int deviceCount = NativeMethods.rs2_get_sensors_count(m_instance, out error);
                return deviceCount;
            }
        }

        public Sensor this[int index]
        {
            get
            {
                object error;
                var ptr = NativeMethods.rs2_create_sensor(m_instance, index, out error);
                return new Sensor(ptr);
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
                NativeMethods.rs2_delete_sensor_list(m_instance);
                m_instance = IntPtr.Zero;

                disposedValue = true;
            }
        }

        //TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~SensorList()
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
    public struct ROI
    {
        public int minX, minY, maxX, maxY;
    }

    public class AutoExposureROI
    {
        internal IntPtr m_instance;

        public ROI GetRegionOfInterest()
        {
            var result = new ROI();
            object error;
            NativeMethods.rs2_get_region_of_interest(m_instance, out result.minX,
                out result.minY, out result.maxX, out result.maxY, out error);
            return result;
        }

        public void SetRegionOfInterest(ROI value)
        {
            object error;
            NativeMethods.rs2_set_region_of_interest(m_instance,
                value.minX, value.minY, value.maxX, value.maxY, out error);
        }
    }

    public class Sensor : IOptions, IDisposable
    {
        protected readonly IntPtr m_instance;
        
        public IntPtr Instance 
        {
            get { return m_instance; }
        }

        internal Sensor(IntPtr sensor)
        {
            //if (sensor == IntPtr.Zero)
            //    throw new ArgumentNullException();
            m_instance = sensor;
        }

        public AutoExposureROI AutoExposureSettings
        {
            get
            {
                object error;
                if (NativeMethods.rs2_is_sensor_extendable_to(m_instance, Extension.Roi, out error) > 0)
                {
                    return new AutoExposureROI { m_instance = m_instance };
                }
                return null;
            }
        }

        public class CameraInfos
        {
            readonly IntPtr m_sensor;
            public CameraInfos(IntPtr sensor) { m_sensor = sensor; }

            public string this[CameraInfo info]
            {
                get
                {
                    object err;
                    if (NativeMethods.rs2_supports_sensor_info(m_sensor, info, out err) > 0)
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_sensor_info(m_sensor, info, out err));
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


        internal class CameraOption : IOption
        {
            readonly IntPtr m_sensor;
            readonly Option option;

            private readonly float min;
            private readonly float max;
            private readonly float step;
            private readonly float @default;
            private string description;

            public CameraOption(IntPtr sensor, Option option)
            {
                m_sensor = sensor;
                this.option = option;

                if (Supported)
                {
                    object error;
                    NativeMethods.rs2_get_option_range(m_sensor, option, out min, out max, out step, out @default, out error);
                }
            }

            public bool Supported
            {
                get
                {
                    try
                    {
                        object error;
                        return NativeMethods.rs2_supports_option(m_sensor, option, out error) > 0;
                    }
                    catch (Exception)
                    {
                        return false;
                    }
                }
            }

            public Option Key
            {
                get
                {
                    return option;
                }
            }

            public string Description
            {
                get
                {
                    if(description == null)
                    {
                        object error;
                        var str = NativeMethods.rs2_get_option_description(m_sensor, option, out error);
                        description = Marshal.PtrToStringAnsi(str);
                    }
                    return description;
                }
            }

            public float Value
            {
                get
                {
                    object error;
                    return NativeMethods.rs2_get_option(m_sensor, option, out error);
                }
                set
                {
                    object error;
                    NativeMethods.rs2_set_option(m_sensor, option, value, out error);
                }
            }

            public string ValueDescription
            {
                get
                {
                    return GetValueDescription(Value);
                }
            }

            public string GetValueDescription(float value)
            {
                object error;
                var str = NativeMethods.rs2_get_option_value_description(m_sensor, option, value, out error);
                return Marshal.PtrToStringAnsi(str);
            }

            public float Min
            {
                get
                {
                    return min;
                }
            }

            public float Max
            {
                get
                {
                    return max;
                }
            }

            public float Step
            {
                get
                {
                    return step;
                }
            }

            public float Default
            {
                get
                {
                    return @default;
                }
            }

            public bool ReadOnly
            {
                get
                {
                    object error;
                    return NativeMethods.rs2_is_option_read_only(m_sensor, option, out error) != 0;
                }
            }
        }
        
        public class SensorOptions : IOptionsContainer
        {
            readonly IntPtr m_sensor;
            internal SensorOptions(IntPtr sensor)
            {
                m_sensor = sensor;
            }

            public IOption this[Option option]
            {
                get
                {
                    return new CameraOption(m_sensor, option);
                }
            }

            public string OptionValueDescription(Option option, float value)
            {
                object error;
                var desc = NativeMethods.rs2_get_option_value_description(m_sensor, option, value, out error);
                if(desc != IntPtr.Zero)
                {
                    return Marshal.PtrToStringAnsi(desc);
                }
                return null;
            }

            static readonly Option[] OptionValues = Enum.GetValues(typeof(Option)) as Option[];

            public IEnumerator<IOption> GetEnumerator()
            {

                foreach (var v in OptionValues)
                {
                    if (this[v].Supported)
                        yield return this[v];
                }
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
        }

        SensorOptions m_options;
        public IOptionsContainer Options
        {
            get
            {
                return m_options = m_options ?? new SensorOptions(m_instance);
            }
        }

        /// <summary>
        /// open subdevice for exclusive access, by commiting to a configuration
        /// </summary>
        /// <param name="profile"></param>
        public void Open(StreamProfile profile)
        {
            object error;
            NativeMethods.rs2_open(m_instance, profile.m_instance.Handle, out error);

        }

        /// <summary>
        /// open subdevice for exclusive access, by commiting to composite configuration, specifying one or more stream profiles 
        /// this method should be used for interdendent streams, such as depth and infrared, that have to be configured together
        /// </summary>
        /// <param name="profiles"></param>
        public void Open(params StreamProfile[] profiles)
        {
            object error;
            IntPtr[] handles = new IntPtr[profiles.Length];
            for (int i = 0; i < profiles.Length; i++)
                handles[i] = profiles[i].m_instance.Handle;
            NativeMethods.rs2_open_multiple(m_instance, handles, profiles.Length, out error);
        }

        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_queue(m_instance, queue.m_instance.Handle, out error);
            m_queue = queue;
            m_callback = null;
        }

        //public delegate void FrameCallback<Frame, T>(Frame frame, T user_data);
        public delegate void FrameCallback(Frame frame);

        public void Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = new Frame(f))
                    cb(frame);
            };
            m_callback = cb2;
            m_queue = null;
            NativeMethods.rs2_start(m_instance, cb2, IntPtr.Zero, out error);
        }

        public void Stop()
        {
            object error;
            NativeMethods.rs2_stop(m_instance, out error);
            m_callback = null;
            m_queue = null;
        }

        /// <summary>
        /// close subdevice for exclusive access this method should be used for releasing device resource
        /// </summary>
        public void Close()
        {
            object error;
            NativeMethods.rs2_close(m_instance, out error);
        }

        /// <summary>
        /// retrieve mapping between the units of the depth image and meters
        /// </summary>
        /// <returns> depth in meters corresponding to a depth value of 1</returns>
        public float DepthScale
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_depth_scale(m_instance, out error);
            }
        }


        public StreamProfileList StreamProfiles
        {
            get
            {
                object error;
                return new StreamProfileList(NativeMethods.rs2_get_stream_profiles(m_instance, out error));
            }
        }


        public IEnumerable<VideoStreamProfile> VideoStreamProfiles
        {
            get
            {
                return StreamProfiles.OfType<VideoStreamProfile>();
            }
        }

        private frame_callback m_callback;
        private FrameQueue m_queue;

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                    m_callback = null;
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                NativeMethods.rs2_delete_sensor(m_instance);

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Sensor()
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
