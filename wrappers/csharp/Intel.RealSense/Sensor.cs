using Intel.RealSense.Frames;
using Intel.RealSense.Profiles;
using Intel.RealSense.Types;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class Sensor : IOptions, IDisposable
    {
        /// <summary>
        /// retrieve mapping between the units of the depth image and meters
        /// </summary>
        /// <returns> depth in meters corresponding to a depth value of 1</returns>
        public float DepthScale
            => NativeMethods.rs2_get_depth_scale(instance, out var error);

        public IntPtr Instance => instance;
        public CameraInfos Info
        {
            get
            {
                if (info == null)
                    info = new CameraInfos(instance);
                return info;
            }
        }
        public IOptionsContainer Options => options = options ?? new SensorOptions(instance);
        public StreamProfileList StreamProfiles
           => new StreamProfileList(NativeMethods.rs2_get_stream_profiles(instance, out var error));
        public IEnumerable<VideoStreamProfile> VideoStreamProfiles
            => StreamProfiles.OfType<VideoStreamProfile>();

        //public delegate void FrameCallback<Frame, T>(Frame frame, T user_data);
        public delegate void FrameCallback(Frame frame);

        protected readonly IntPtr instance;

        private CameraInfos info;
        private SensorOptions options;
        private FrameCallbackHandler callback;
        private FrameQueue queue;

        internal Sensor(IntPtr sensor)
        {
            //if (sensor == IntPtr.Zero)
            //    throw new ArgumentNullException();
            instance = sensor;
        }

        /// <summary>
        /// open subdevice for exclusive access, by commiting to a configuration
        /// </summary>
        /// <param name="profile"></param>
        public void Open(StreamProfile profile)
            => NativeMethods.rs2_open(instance, profile.Instance.Handle, out var error);

        /// <summary>
        /// open subdevice for exclusive access, by commiting to composite configuration, specifying one or more stream profiles 
        /// this method should be used for interdendent streams, such as depth and infrared, that have to be configured together
        /// </summary>
        /// <param name="profiles"></param>
        public void Open(params StreamProfile[] profiles)
        {
            IntPtr[] handles = new IntPtr[profiles.Length];
            for (int i = 0; i < profiles.Length; i++)
                handles[i] = profiles[i].Instance.Handle;
            NativeMethods.rs2_open_multiple(instance, handles, profiles.Length, out var error);
        }

        public void Start(FrameQueue queue)
        {
            NativeMethods.rs2_start_queue(instance, queue.Instance.Handle, out var error);
            this.queue = queue;
            callback = null;
        }

        public void Start(FrameCallback cb)
        {
            void cb2(IntPtr f, IntPtr u)
            {
                using (var frame = new Frame(f))
                    cb(frame);
            }
            callback = cb2;
            queue = null;
            NativeMethods.rs2_start(instance, cb2, IntPtr.Zero, out var error);
        }

        public void Stop()
        {
            NativeMethods.rs2_stop(instance, out var error);
            callback = null;
            queue = null;
        }

        /// <summary>
        /// close subdevice for exclusive access this method should be used for releasing device resource
        /// </summary>
        public void Close()
            => NativeMethods.rs2_close(instance, out var error);

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                    callback = null;
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                NativeMethods.rs2_delete_sensor(instance);

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

        public class SensorOptions : IOptionsContainer
        {
            private static readonly Option[] OptionValues;

            private readonly IntPtr sensor;

            static SensorOptions()
            {
                OptionValues = Enum.GetValues(typeof(Option)) as Option[];
            }

            internal SensorOptions(IntPtr sensor)
            {
                this.sensor = sensor;
            }

            public IOption this[Option option] => new CameraOption(sensor, option);
            public string OptionValueDescription(Option option, float value)
            {
                var desc = NativeMethods.rs2_get_option_value_description(sensor, option, value, out var error);
                if (desc != IntPtr.Zero)
                {
                    return Marshal.PtrToStringAnsi(desc);
                }
                return null;
            }

            public IEnumerator<IOption> GetEnumerator()
            {

                foreach (var v in OptionValues)
                {
                    if (this[v].Supported)
                        yield return this[v];
                }
            }

            IEnumerator IEnumerable.GetEnumerator()
                => GetEnumerator();
        }

        internal class CameraOption : IOption
        {
            public bool Supported
            {
                get
                {
                    try
                    {
                        return NativeMethods.rs2_supports_option(sensor, Key, out var error) > 0;
                    }
                    catch (Exception)
                    {
                        return false;
                    }
                }
            }

            public Option Key { get; }
            public string Description {
                get
                {
                    if (description == null)
                    {
                        object error;
                        var str = NativeMethods.rs2_get_option_description(sensor, Key, out error);
                        description = Marshal.PtrToStringAnsi(str);
                    }
                    return description;
                }
            }
            public float Value
            {
                get => NativeMethods.rs2_get_option(sensor, Key, out var error);
                set => NativeMethods.rs2_set_option(sensor, Key, value, out var error);
            }
            public string ValueDescription => GetValueDescription(Value);
            public string GetValueDescription(float value)
            {
                var str = NativeMethods.rs2_get_option_value_description(sensor, Key, value, out var error);
                return Marshal.PtrToStringAnsi(str);
            }
            public float Min => min;
            public float Max => max;
            public float Step => step;
            public float Default => @default;
            public bool ReadOnly => NativeMethods.rs2_is_option_read_only(sensor, Key, out var error) != 0;

            private string description;

            private readonly IntPtr sensor;
            private readonly float min;
            private readonly float max;
            private readonly float step;
            private readonly float @default;
            
            public CameraOption(IntPtr sensor, Option option)
            {
                this.sensor = sensor;
                Key = option;

                if (Supported)
                {
                    NativeMethods.rs2_get_option_range(this.sensor, option, out min, out max, out step, out @default, out var error);
                }
            }
        }

        public class CameraInfos
        {
            public string this[CameraInfo info]
            {
                get
                {
                    if (NativeMethods.rs2_supports_sensor_info(sensor, info, out var err) > 0)
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_sensor_info(sensor, info, out err));
                    return null;
                }
            }

            private readonly IntPtr sensor;

            public CameraInfos(IntPtr sensor)
            {
                this.sensor = sensor;
            }
        }
    }
}
