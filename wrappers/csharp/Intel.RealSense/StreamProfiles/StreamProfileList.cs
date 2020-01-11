// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;

    internal sealed class StreamProfileList : Base.Object, IEnumerable<StreamProfile>, ICollection
    {
        public StreamProfileList(IntPtr ptr)
            : base(ptr, NativeMethods.rs2_delete_stream_profiles_list)
        {
        }

        /// <inheritdoc/>
        public IEnumerator<StreamProfile> GetEnumerator()
        {
            object error;

            int size = NativeMethods.rs2_get_stream_profiles_count(Handle, out error);
            for (int i = 0; i < size; i++)
            {
                var ptr = NativeMethods.rs2_get_stream_profile(Handle, i, out error);
                yield return StreamProfile.Create(ptr);
            }
        }

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>Gets the number of supported stream profiles</summary>
        /// <value>number of supported subdevice profiles</value>
        public int Count
        {
            get
            {
                object error;
                return NativeMethods.rs2_get_stream_profiles_count(Handle, out error);
            }
        }

        /// <inheritdoc/>
        public object SyncRoot => this;

        /// <inheritdoc/>
        public bool IsSynchronized => false;

        /// <summary>
        /// Gets a specific stream profile
        /// </summary>
        /// <param name="index">the zero based index of the streaming mode</param>
        /// <returns>stream profile at given index</returns>
        public StreamProfile this[int index]
        {
            get
            {
                return GetProfile<StreamProfile>(index);
            }
        }

        /// <summary>
        /// Gets a specific stream profile
        /// </summary>
        /// <param name="index">the zero based index of the streaming mode</param>
        /// <typeparam name="T">type of StreamProfile or a subclass</typeparam>
        /// <returns>stream profile at given index</returns>
        public T GetProfile<T>(int index)
            where T : StreamProfile
        {
            object error;
            return StreamProfile.Create<T>(NativeMethods.rs2_get_stream_profile(Handle, index, out error));
        }

        /// <inheritdoc/>
        public void CopyTo(Array array, int index)
        {
            if (array == null)
            {
                throw new ArgumentNullException("array");
            }

            for (int i = 0; i < Count; i++)
            {
                array.SetValue(this[i], i + index);
            }
        }
    }
}
