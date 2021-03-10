// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices;

    // TODO: subclasses - DepthSensor, DepthStereoSensor, PoseSensor...
    public class Sensor : Base.RefCountedPooledObject, IOptions
    {
        protected static Hashtable refCountTable = new Hashtable();
        protected static readonly object tableLock = new object();

        internal override void Initialize()
        {
            lock (tableLock)
            {
                if (refCountTable.Contains(Handle))
                    refCount = refCountTable[Handle] as Base.RefCount;
                else
                {
                    refCount = new Base.RefCount();
                    refCountTable[Handle] = refCount;
                }
                Retain();
            }
            Info = new InfoCollection(NativeMethods.rs2_supports_sensor_info, NativeMethods.rs2_get_sensor_info, Handle);
            Options = new OptionsList(Handle);
        }

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private frame_callback m_callback;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private FrameQueue m_queue;

        internal static T Create<T>(IntPtr ptr)
            where T : Sensor
        {
            return ObjectPool.Get<T>(ptr);
        }

        /// <summary>Returns a strongly-typed clone</summary>
        /// <typeparam name="T"><see cref="Sensor"/> type or subclass</typeparam>
        /// <param name="other"><see cref="Sensor"/> to clone</param>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public static T Create<T>(Sensor other)
            where T : Sensor
        {
            object error;
            return ObjectPool.Get<T>(other.Handle);
        }

        internal Sensor(IntPtr sensor)
            : base(sensor, NativeMethods.rs2_delete_sensor)
        {
            Initialize();
        }

        protected override void Dispose(bool disposing)
        {
            if (m_instance.IsInvalid)
            {
                return;
            }

            //m_queue.Dispose();
            (Options as OptionsList).Dispose();

            lock (tableLock)
            {
                IntPtr localHandle = Handle;
                System.Diagnostics.Debug.Assert(refCountTable.Contains(localHandle));

                base.Dispose(disposing);

                if (refCount.count == 0)
                {
                    refCountTable.Remove(localHandle);
                }
            }
        }

        public class CameraInfos : IEnumerable<KeyValuePair<CameraInfo, string>>
        {
            private readonly IntPtr m_sensor;

            public CameraInfos(IntPtr sensor)
            {
                m_sensor = sensor;
            }

            /// <summary>retrieve sensor specific information, like versions of various internal components</summary>
            /// <param name="info">camera info type to retrieve</param>
            /// <returns>the requested camera info string, in a format specific to the device model</returns>
            public string this[CameraInfo info]
            {
                get
                {
                    object err;
                    if (NativeMethods.rs2_supports_sensor_info(m_sensor, info, out err) > 0)
                    {
                        return Marshal.PtrToStringAnsi(NativeMethods.rs2_get_sensor_info(m_sensor, info, out err));
                    }

                    return null;
                }
            }

            private static readonly CameraInfo[] CameraInfoValues = Enum.GetValues(typeof(CameraInfo)) as CameraInfo[];

            /// <inheritdoc/>
            public IEnumerator<KeyValuePair<CameraInfo, string>> GetEnumerator()
            {
                object error;
                foreach (var key in CameraInfoValues)
                {
                    if (NativeMethods.rs2_supports_sensor_info(m_sensor, key, out error) > 0)
                    {
                        var value = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_sensor_info(m_sensor, key, out error));
                        yield return new KeyValuePair<CameraInfo, string>(key, value);
                    }
                }
            }

            /// <inheritdoc/>
            IEnumerator IEnumerable.GetEnumerator()
            {
                return GetEnumerator();
            }
        }

        public InfoCollection Info { get; private set; }


        /// <inheritdoc/>
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
            {
                handles[i] = profiles[i].Handle;
            }

            NativeMethods.rs2_open_multiple(Handle, handles, profiles.Length, out error);
        }

        /// <summary>
        /// start streaming from specified configured sensor of specific stream to frame queue
        /// </summary>
        /// <param name="queue">frame-queue to store new frames into</param>
        // TODO: overload with state object and Action<Frame, object> callback to avoid allocations
        public void Start(FrameQueue queue)
        {
            object error;
            NativeMethods.rs2_start_queue(Handle, queue.Handle, out error);
            m_queue = queue;
            m_callback = null;
        }

        /// <summary>start streaming from specified configured sensor</summary>
        /// <param name="cb">delegate to register as per-frame callback</param>
        // TODO: overload with state object and Action<Frame, object> callback to avoid allocations
        public void Start(FrameCallback cb)
        {
            object error;
            frame_callback cb2 = (IntPtr f, IntPtr u) =>
            {
                using (var frame = Frame.Create(f))
                {
                    cb(frame);
                }
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

        public bool Is(Extension ext)
        {
            object error;
            return NativeMethods.rs2_is_sensor_extendable_to(Handle, ext, out error) != 0;
        }

        /// <summary>Returns a strongly-typed clone</summary>
        /// <typeparam name="T"><see cref="Sensor"/> type or subclass</typeparam>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public T As<T>()
            where T : Sensor
        {
            return Create<T>(this);
        }

        /// <summary>
        /// Gets the mapping between the units of the depth image and meters
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

        /// <summary>
        /// Gets the active region of interest to be used by auto-exposure algorithm
        /// </summary>
        public AutoExposureROI AutoExposureSettings
        {
            get
            {
                object error;
                if (NativeMethods.rs2_is_sensor_extendable_to(Handle, Extension.Roi, out error) != 0)
                {
                    return new AutoExposureROI { m_instance = Handle };
                }

                // TODO: consider making AutoExposureROI a struct and throwing instead
                return null;
            }
        }

        /// <summary>Gets the list of supported stream profiles</summary>
        /// <value>list of stream profiles that given subdevice can provide</value>
        public ReadOnlyCollection<StreamProfile> StreamProfiles
        {
            get
            {
                object error;
                using (var pl = new StreamProfileList(NativeMethods.rs2_get_stream_profiles(Handle, out error)))
                {
                    var profiles = new StreamProfile[pl.Count];
                    pl.CopyTo(profiles, 0);
                    return Array.AsReadOnly(profiles);
                }
            }
        }

        /// <summary>Gets the list of recommended processing blocks for a specific sensor.</summary>
        /// <remarks>
        /// Order and configuration of the blocks are decided by the sensor
        /// </remarks>
        /// <value>list of supported sensor recommended processing blocks</value>
        public ReadOnlyCollection<ProcessingBlock> ProcessingBlocks
        {
            get
            {
                object error;
                using (var pl = new ProcessingBlockList(NativeMethods.rs2_get_recommended_processing_blocks(Handle, out error)))
                {
                    var blocks = new ProcessingBlock[pl.Count];
                    pl.CopyTo(blocks, 0);
                    return Array.AsReadOnly(blocks);
                }
            }
        }
    }
}
