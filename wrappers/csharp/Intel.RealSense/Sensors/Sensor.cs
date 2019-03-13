using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
    public class Sensor : Base.PooledObject, IOptions
    {
        internal override void Initialize()
        {
            Info = new CameraInfos(Handle);
            Options = new SensorOptions(Handle);
        }

        private frame_callback m_callback;
        private FrameQueue m_queue;

        internal static T Create<T>(IntPtr ptr) where T : Sensor
        {
            return Pool.Get<T>(ptr);
        }

        internal Sensor(IntPtr sensor) : base(sensor, NativeMethods.rs2_delete_sensor)
        {
            Info = new CameraInfos(Handle);
            Options = new SensorOptions(Handle);
        }

        public class CameraInfos
        {
            readonly IntPtr m_sensor;
            public CameraInfos(IntPtr sensor) { m_sensor = sensor; }

            /// <summary>retrieve sensor specific information, like versions of various internal components</summary>
            /// <param name="info">camera info type to retrieve</param>
            /// <returns>the requested camera info string, in a format specific to the device model</returns>
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

        public CameraInfos Info { get; private set; }

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


            /// <summary>check if particular option is supported by a subdevice</summary>
            /// <value>true if option is supported</value>
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

            /// <summary>get option description</summary>
            /// <value>human-readable option description</value>
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


            /// <summary>Read and write option value</summary>
            /// <value>value of the option</value>
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

            /// <summary>get option value description (in case specific option value hold special meaning)</summary>
            /// <value>human-readable description of a specific value of an option or null if no special meaning</value>
            public string ValueDescription
            {
                get
                {
                    return GetValueDescription(Value);
                }
            }

            /// <summary>get option value description (in case specific option value hold special meaning)</summary>
            /// <param name="value">value of the option</param>
            /// <returns>human-readable description of a specific value of an option or null if no special meaning</returns>
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

            /// <summary>check if an option is read-only</summary>
            /// <value>true if option is read-only</value>
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

            /// <summary>read option value from the sensor</summary>
            /// <param name="option">option id to be queried</param>
            /// <value>value of the option</value>
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

        public IOptionsContainer Options { get; private set; }

        /// <summary>open subdevice for exclusive access, by committing to a configuration</summary>
        /// <param name="profile">stream profile that defines single stream configuration</param>
        public void Open(StreamProfile profile)
        {
            object error;
            NativeMethods.rs2_open(Handle, profile.Handle, out error);

        }

        /// <summary>open subdevice for exclusive access, by committing to composite configuration, specifying one or more stream profiles</summary>
        /// <remarks>
        /// this method should be used for interdependent streams, such as depth and infrared, that have to be configured together
        /// </remarks>
        /// <param name="profiles">list of stream profiles</param>
        public void Open(params StreamProfile[] profiles)
        {
            object error;
            IntPtr[] handles = new IntPtr[profiles.Length];
            for (int i = 0; i < profiles.Length; i++)
                handles[i] = profiles[i].Handle;
            NativeMethods.rs2_open_multiple(Handle, handles, profiles.Length, out error);
        }

        /// <summary>
        /// start streaming from specified configured sensor of specific stream to frame queue
        /// </summary>
        /// <param name="queue">frame-queue to store new frames into</param>
        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_queue(Handle, queue.Handle, out error);
            m_queue = queue;
            m_callback = null;
        }

        /// <summary>start streaming from specified configured sensor</summary>
        /// <param name="cb">delegate to register as per-frame callback</param>
        public void Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                    cb(frame);
            };
            m_callback = cb2;
            m_queue = null;
            NativeMethods.rs2_start(Handle, cb2, IntPtr.Zero, out error);
        }

        /// <summary>start streaming from specified configured sensor</summary>
        /// <param name="cb">delegate to register as per-frame callback</param>
        public void Start<T>(Action<T> cb) where T : Frame
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create<T>(f))
                    cb(frame);
            };
            m_callback = cb2;
            m_queue = null;
            NativeMethods.rs2_start(Handle, cb2, IntPtr.Zero, out error);
        }

        /// <summary>
        /// stops streaming from specified configured device
        /// </summary>
        public void Stop()
        {
            object error;
            NativeMethods.rs2_stop(Handle, out error);
            m_callback = null;
            m_queue = null;
        }

        /// <summary>
        /// close subdevice for exclusive access this method should be used for releasing device resource
        /// </summary>
        public void Close()
        {
            object error;
            NativeMethods.rs2_close(Handle, out error);
        }

        #region Extensions

        /// <summary>
        /// retrieve mapping between the units of the depth image and meters
        /// </summary>
        /// <value>depth in meters corresponding to a depth value of 1</value>
        public float DepthScale
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_depth_scale(Handle, out error);
            }
        }

        public AutoExposureROI AutoExposureSettings
        {
            get
            {
                object error;
                if (NativeMethods.rs2_is_sensor_extendable_to(Handle, Extension.Roi, out error) == 0)
                {
                    return new AutoExposureROI { m_instance = Handle };
                }
                return null;
            }
        }
        #endregion

        /// <value>list of stream profiles that given subdevice can provide</value>
        public StreamProfileList StreamProfiles
        {
            get
            {
                object error;
                return new StreamProfileList(NativeMethods.rs2_get_stream_profiles(Handle, out error));
            }
        }
    }
}
