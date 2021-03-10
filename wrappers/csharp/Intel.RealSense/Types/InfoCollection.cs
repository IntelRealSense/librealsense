// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

using System.Collections.Generic;
using System.Diagnostics;
using Intel.RealSense;

[assembly: DebuggerDisplay("{Value,nq}", Name = "{Key}", Target = typeof(KeyValuePair<CameraInfo, string>))]

namespace Intel.RealSense
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Security;

    [DebuggerTypeProxy(typeof(InfoCollectionDebugView))]
    [DebuggerDisplay("Count = {Count}")]
    public class InfoCollection : IEnumerable<KeyValuePair<CameraInfo, string>>
    {
        internal sealed class InfoCollectionDebugView
        {
            private readonly InfoCollection ic;

            [DebuggerBrowsable(DebuggerBrowsableState.RootHidden)]
            public KeyValuePair<CameraInfo, string>[] Items => ic.ToArray();

            public InfoCollectionDebugView(InfoCollection optionList)
            {
                if (optionList == null)
                {
                    throw new ArgumentNullException(nameof(optionList));
                }

                ic = optionList;
            }
        }

        public int Count => Enumerable.Count(this);

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly SupportsDelegate supports;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly GetInfoDelegate get;

        [DebuggerBrowsable(DebuggerBrowsableState.Never)]
        private readonly IntPtr handle;

        [SuppressUnmanagedCodeSecurity]
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate IntPtr GetInfoDelegate(IntPtr obj, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ErrorMarshaler))] out object error);

        [SuppressUnmanagedCodeSecurity]
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        internal delegate int SupportsDelegate(IntPtr obj, CameraInfo info, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ErrorMarshaler))] out object error);

        internal InfoCollection(SupportsDelegate supports, GetInfoDelegate get, IntPtr handle)
        {
            this.supports = supports;
            this.get = get;
            this.handle = handle;
        }

        /// <summary>
        /// retrieve camera specific information, like versions of various internal components
        /// </summary>
        /// <param name="info">camera info type to retrieve</param>
        /// <returns>the requested camera info string, in a format specific to the device model</returns>
        public string GetInfo(CameraInfo info)
        {
            object error;
            return Marshal.PtrToStringAnsi(get(handle, info, out error));
        }

        /// <summary>
        /// check if specific camera info is supported
        /// </summary>
        /// <param name="info">the parameter to check for support</param>
        /// <returns>true if the parameter both exist and well-defined for the specific device</returns>
        public bool Supports(CameraInfo info)
        {
            object error;
            return supports(handle, info, out error) > 0;
        }

        /// <summary>
        /// retrieve camera specific information, like versions of various internal components
        /// </summary>
        /// <param name="info">camera info type to retrieve</param>
        /// <returns>the requested camera info string, in a format specific to the device model</returns>
        public string this[CameraInfo info] => Supports(info) ? GetInfo(info) : null;

        private static readonly CameraInfo[] CameraInfoValues = Enum.GetValues(typeof(CameraInfo)) as CameraInfo[];

        /// <inheritdoc/>
        public IEnumerator<KeyValuePair<CameraInfo, string>> GetEnumerator()
        {
            foreach (var info in CameraInfoValues)
            {
                if (Supports(info))
                {
                    yield return new KeyValuePair<CameraInfo, string>(info, GetInfo(info));
                }
            }
        }

        /// <inheritdoc/>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
