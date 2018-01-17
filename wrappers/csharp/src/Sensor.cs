using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;

namespace Intel.RealSense
{
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

    public class Sensor
    {
        public IntPtr m_instance;

        internal Sensor(IntPtr dev)
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


        public class CameraOption
        {
            IntPtr dev;
            Option option;

            private readonly float min;
            private readonly float max;
            private readonly float step;
            private readonly float @default;

            public CameraOption(IntPtr dev, Option option)
            {
                this.dev = dev;
                this.option = option;

                if (Supported)
                {
                    object error;
                    NativeMethods.rs2_get_option_range(dev, option, out min, out max, out step, out @default, out error);
                }
            }

            public bool Supported
            {
                get
                {
                    try
                    {
                        object error;
                        return NativeMethods.rs2_supports_option(dev, option, out error) > 0;
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

            public float Value
            {
                get
                {
                    object error;
                    return NativeMethods.rs2_get_option(dev, option, out error);
                }
                set
                {
                    object error;
                    NativeMethods.rs2_set_option(dev, option, value, out error);
                }
            }

            public string ValueDescription
            {
                get
                {
                    object error;
                    var str = NativeMethods.rs2_get_option_value_description(dev, option, Value, out error);
                    return Marshal.PtrToStringAnsi(str);
                }
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
        }

        public class SensorOptions : IEnumerable<CameraOption>
        {
            IntPtr dev;
            public SensorOptions(IntPtr dev)
            {
                this.dev = dev;
            }

            public CameraOption this[Option option]
            {
                get
                {
                    return new CameraOption(dev, option);
                }
            }

            public IEnumerator<CameraOption> GetEnumerator()
            {
                var values = Enum.GetValues(typeof(Option)) as Option[];
                foreach (var v in values)
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
        public SensorOptions Options
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
        }

        //public delegate void FrameCallback<Frame, T>(Frame frame, T user_data);
        public delegate void FrameCallback(Frame frame);

        public void Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                //!TODO: check whether or not frame should be diposed here... I've got 2 concerns here:
                // 1. Best practice says that since the user's callback isn't the owner, frame should be diposed here...
                // 2. Users might need the frame for longer, but then they could clone it

                //cb(new Frame(f));
                using (var frame = new Frame(f))
                    cb(frame);
            };
            NativeMethods.rs2_start(m_instance, cb2, IntPtr.Zero, out error);
        }

        public void Stop()
        {
            object error;
            NativeMethods.rs2_stop(m_instance, out error);
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
                return StreamProfiles.Where(p => p is VideoStreamProfile).Select(p => p as VideoStreamProfile);
            }
        }


        //public Intrinsics GetIntrinsics(StreamProfile profile)
        //{
        //    object error;
        //    Intrinsics intrinsics;
        //    NativeMethods.rs_get_stream_intrinsics(m_instance, profile.stream, profile.width, profile.height, profile.fps, profile.format, out intrinsics, out error);
        //    return intrinsics;
        //}


        //public Extrinsics GetExtrinsicsTo(Device to_device)
        ////public Extrinsics GetExtrinsicsTo(RS_STREAM from_stream, Device to_device, RS_STREAM to_stream)
        ////public Extrinsics GetExtrinsicsTo(RS_STREAM from_stream, RS_STREAM to_stream)
        //{
        //    object error;
        //    Extrinsics extrinsics;
        //    NativeMethods.rs_get_extrinsics(m_instance, to_device.m_instance, out extrinsics, out error);
        //    //NativeMethods.rs_get_extrinsics(m_instance, from_stream, to_device.m_instance, to_stream, out extrinsics, out error);
        //    //NativeMethods.rs_get_extrinsics(m_instance, from_stream, to_stream, out extrinsics, out error);
        //    return extrinsics;
        //}
    }

}
