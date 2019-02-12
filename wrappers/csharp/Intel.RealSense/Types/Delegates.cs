using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_callback(IntPtr frame, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_processor_callback(IntPtr frame, IntPtr frame_src, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_deleter(IntPtr frame);
    
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void rs2_devices_changed_callback(IntPtr removed, IntPtr added, IntPtr user_data);
}
