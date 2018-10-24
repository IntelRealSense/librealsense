using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense.Types
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void FrameCallbackHandler(IntPtr frame, IntPtr user_data);
    //public delegate void frame_callback([Out, MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(FrameMarshaler))] out Frame frame, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void FrameProcessorCallbackHandler(IntPtr frame, IntPtr frame_src, IntPtr user_data);
    //fram_processor_callback

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_deleter(IntPtr frame, IntPtr user_data);
    
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void rs2_devices_changed_callback(IntPtr removed, IntPtr added, IntPtr user_data);       
}
