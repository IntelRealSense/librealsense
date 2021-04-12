// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma warning disable SA1600 // Elements must be documented
#pragma warning disable SA1124 // Do not use regions

namespace Intel.RealSense.Net
{
    using System;
    using System.Reflection.Emit;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;

    [SuppressUnmanagedCodeSecurity]
    [SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
    internal static class NativeMethods
    {

#if DEBUG
        private const string dllName = "realsense2d";
        private const string dllNetName = "realsense2-netd";
#else
        private const string dllName = "realsense2";
        private const string dllNetName = "realsense2-net";
#endif

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void rs2_context_add_software_device(ContextHandle ctx, IntPtr device, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ErrorMarshaler))] out object error);

        [DllImport(dllName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int rs2_get_api_version([MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ErrorMarshaler))] out object error);

        #region rs_net_device
        [DllImport(dllNetName, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr rs2_create_net_device(int api_version, string address, [MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(ErrorMarshaler))] out object error);
        #endregion
    }
}

#pragma warning restore SA1600 // Elements must be documented
#pragma warning restore SA1124 // Do not use regions
