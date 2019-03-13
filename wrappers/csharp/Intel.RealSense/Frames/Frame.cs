using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Base class for multiple frame extensions
    /// </summary>
    public class Frame : Base.PooledObject
    {
        internal override void Initialize()
        {
        }

        internal Frame(IntPtr ptr) : base(ptr, NativeMethods.rs2_release_frame)
        {
        }

        public static Frame Create(IntPtr ptr)
        {
            return Create<Frame>(ptr);
        }

        public static T Create<T>(IntPtr ptr) where T : Frame
        {
            return Pool.Get<T>(ptr);
        }

        public static T Create<T>(Frame other) where T : Frame
        {
            object error;
            NativeMethods.rs2_frame_add_ref(other.Handle, out error);
            return Pool.Get<T>(other.Handle);
        }

        /// <summary>Test if the given frame can be extended to the requested extension</summary>
        /// <param name="extension">The extension to which the frame should be tested if it is extendable</param>
        /// <returns>true iff the frame can be extended to the given extension</returns>
        public bool Is(Extension extension)
        {
            object error;
            return NativeMethods.rs2_is_frame_extendable_to(Handle, extension, out error) != 0;
        }

        /// <summary>Cast current instance as the type of another class</summary>
        /// <typeparam name="T">New frame subtype</typeparam>
        /// <returns>class instance</returns>
        public T As<T>() where T : Frame
        {
            return Create<T>(this);
        }

        public Frame Clone()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(Handle, out error);
            return Create(Handle);
        }

        /// <summary>communicate to the library you intend to keep the frame alive for a while
        /// this will remove the frame from the regular count of the frame pool</summary>
        /// <remarks>once this function is called, the SDK can no longer guarantee 0-allocations during frame cycling</remarks>
        public void Keep() {
            NativeMethods.rs2_keep_frame(Handle);
        }

        /// <summary>
        /// Test if frame is composite
        /// <para>Shorthand for <c>Is(Extension.CompositeFrame)</c></para>
        /// </summary>
        /// <value>true if frame is a composite frame and false otherwise</value>
        public bool IsComposite
        {
            get
            {
                return Is(Extension.CompositeFrame);
            }
        }

        /// <summary>retrieve data from frame handle</summary>
        /// <value>pointer to the start of the frame data</value>
        public IntPtr Data
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_data(Handle, out error);
            }
        }

        public T GetProfile<T>() where T : StreamProfile
        {
            object error;
            var ptr = NativeMethods.rs2_get_frame_stream_profile(Handle, out error);
            return StreamProfile.Create<T>(ptr);
        }

        public StreamProfile Profile => GetProfile<StreamProfile>();

        /// <summary>retrieve frame number from frame handle</summary>
        /// <value>the frame nubmer of the frame</value>
        public ulong Number
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_number(Handle, out error);
            }
        }

        /// <summary>retrieve timestamp from frame handle in milliseconds</summary>
        /// <value>the timestamp of the frame in milliseconds</value>
        public double Timestamp
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_timestamp(Handle, out error);
            }
        }

        /// <summary>retrieve timestamp domain from frame handle. timestamps can only be comparable if they are in common domain</summary>
        /// <remarks>
        /// (for example, depth timestamp might come from system time while color timestamp might come from the device)
        /// this method is used to check if two timestamp values are comparable (generated from the same clock)
        /// </remarks>
        /// <value>the timestamp domain of the frame (camera / microcontroller / system time)</value>
        public TimestampDomain TimestampDomain
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_timestamp_domain(Handle, out error);
            }
        }

        public long this[FrameMetadataValue frame_metadata]
        {
            get {
                return GetFrameMetadata(frame_metadata);
            }
        }

        /// <summary>retrieve metadata from frame handle</summary>
        /// <param name="frame_metadata">the <see cref="FrameMetadataValue">FrameMetadataValue</see> whose latest frame we are interested in</param>
        /// <returns>the metadata value</returns>
        public long GetFrameMetadata(FrameMetadataValue frame_metadata)
        {
            object error;
            return NativeMethods.rs2_get_frame_metadata(Handle, frame_metadata, out error);
        }

        /// <summary>determine device metadata</summary>
        /// <param name="frame_metadata">the metadata to check for support</param>
        /// <returns>true if device has this metadata</returns>
        public bool SupportsFrameMetaData(FrameMetadataValue frame_metadata)
        {
            object error;
            return NativeMethods.rs2_supports_frame_metadata(Handle, frame_metadata, out error) != 0;
        }
    }
}
