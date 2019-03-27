// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Base class for multiple frame extensions
    /// </summary>
    public class Frame : Base.PooledObject
    {
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private static readonly Base.Deleter FrameReleaser = NativeMethods.rs2_release_frame;

        internal override void Initialize()
        {
        }

        internal Frame(IntPtr ptr)
            : base(ptr, FrameReleaser)
        {
        }

        /// <summary>
        /// Create a frame from a native pointer
        /// </summary>
        /// <param name="ptr">Native <c>rs2_frame*</c> pointer</param>
        /// <returns>a new <see cref="Frame"/></returns>
        public static Frame Create(IntPtr ptr)
        {
            return Create<Frame>(ptr);
        }

        /// <summary>
        /// Create a frame from a native pointer
        /// </summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="ptr">Native <c>rs2_frame*</c> pointer</param>
        /// <returns>a new <typeparamref name="T"/></returns>
        public static T Create<T>(IntPtr ptr)
            where T : Frame
        {
            return ObjectPool.Get<T>(ptr);
        }

        /// <summary>Returns a strongly-typed clone</summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <param name="other"><see cref="Frame"/> to clone</param>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public static T Create<T>(Frame other)
            where T : Frame
        {
            object error;
            NativeMethods.rs2_frame_add_ref(other.Handle, out error);
            return ObjectPool.Get<T>(other.Handle);
        }

        /// <summary>Test if the given frame can be extended to the requested extension</summary>
        /// <param name="extension">The extension to which the frame should be tested if it is extendable</param>
        /// <returns><see langword="true"/> iff the frame can be extended to the given extension</returns>
        public bool Is(Extension extension)
        {
            object error;
            return NativeMethods.rs2_is_frame_extendable_to(Handle, extension, out error) != 0;
        }

        /// <summary>Returns a strongly-typed clone</summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public T As<T>()
            where T : Frame
        {
            return Create<T>(this);
        }

        /// <summary>Returns a strongly-typed clone, <see langword="this"/> is disposed</summary>
        /// <typeparam name="T"><see cref="Frame"/> type or subclass</typeparam>
        /// <returns>an instance of <typeparamref name="T"/></returns>
        public T Cast<T>()
            where T : Frame
        {
            using (this)
            {
                return Create<T>(this);
            }
        }

        /// <summary>
        /// Add a reference to this frame and return a clone, does not copy data
        /// </summary>
        /// <returns>A clone of this frame</returns>
        public Frame Clone()
        {
            object error;
            NativeMethods.rs2_frame_add_ref(Handle, out error);
            return Create(Handle);
        }

        /// <summary>communicate to the library you intend to keep the frame alive for a while
        /// <para>
        /// this will remove the frame from the regular count of the frame pool
        /// </para>
        /// </summary>
        /// <remarks>
        /// once this function is called, the SDK can no longer guarantee 0-allocations during frame cycling
        /// </remarks>
        public void Keep()
        {
            NativeMethods.rs2_keep_frame(Handle);
        }

        /// <summary>
        /// Gets a value indicating whether frame is a composite frame
        /// <para>Shorthand for <c>Is(<see cref="Extension.CompositeFrame"/>)</c></para>
        /// </summary>
        /// <seealso cref="Is(Extension)"/>
        /// <value><see langword="true"/> if frame is a composite frame and false otherwise</value>
        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        public bool IsComposite
        {
            get
            {
                return Is(Extension.CompositeFrame);
            }
        }

        /// <summary>Gets a pointer to the frame data</summary>
        /// <value>pointer to the start of the frame data</value>
        public IntPtr Data
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_data(Handle, out error);
            }
        }

        /// <summary>
        /// Returns the stream profile that was used to start the stream of this frame
        /// </summary>
        /// <typeparam name="T">StreamProfile or subclass type</typeparam>
        /// <returns>the stream profile that was used to start the stream of this frame</returns>
        public T GetProfile<T>()
            where T : StreamProfile
        {
            object error;
            var ptr = NativeMethods.rs2_get_frame_stream_profile(Handle, out error);
            return StreamProfile.Create<T>(ptr);
        }

        /// <summary>
        /// Gets the stream profile that was used to start the stream of this frame
        /// </summary>
        /// <see cref="GetProfile{T}"/>
        public StreamProfile Profile => GetProfile<StreamProfile>();

        /// <summary>Gets the frame number of the frame</summary>
        /// <value>the frame nubmer of the frame</value>
        public ulong Number
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_number(Handle, out error);
            }
        }

        /// <summary>Gets timestamp from frame handle in milliseconds</summary>
        /// <value>the timestamp of the frame in milliseconds</value>
        public double Timestamp
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_frame_timestamp(Handle, out error);
            }
        }

        /// <summary>Gets the timestamp domain from frame handle. timestamps can only be comparable if they are in common domain</summary>
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
            get
            {
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

#if DEBUGGER_METADATA
        private static readonly FrameMetadataValue[] MetadataValues = Enum.GetValues(typeof(FrameMetadataValue)) as FrameMetadataValue[];
        public ICollection<KeyValuePair<FrameMetadataValue, long>> MetaData
        {
            get
            {
                return MetadataValues
                    .Where(m => SupportsFrameMetaData(m))
                    .Select(m => new KeyValuePair<FrameMetadataValue, long>(m, GetFrameMetadata(m)))
                    .ToArray();
            }
        }
#endif
    }
}
