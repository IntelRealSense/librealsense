using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public static class Helpers
    {
        /// <summary>
        /// Custom marshaler for throwing exceptions on errors codes.
        /// </summary>
        public class ErrorMarshaler : ICustomMarshaler
        {
            public static ICustomMarshaler Instance
            {
                get
                {
                    if (instance == null)
                    {
                        instance = new ErrorMarshaler();
                    }
                    return instance;
                }
            }
            //private static ErrorMarshaler Instance = new ErrorMarshaler();
            private static ErrorMarshaler instance;

            //is needed vor marshalling in .NET
            public static ICustomMarshaler GetInstance(string v)
                => Instance;

            public void CleanUpManagedData(object ManagedObj)
            {
            }

            public void CleanUpNativeData(IntPtr pNativeData)
            {
                //!TODO: maybe rs_free_error here?
                NativeMethods.rs2_free_error(pNativeData);
            }

            public int GetNativeDataSize()
                => IntPtr.Size;

            public IntPtr MarshalManagedToNative(object ManagedObj)
                => IntPtr.Zero;

            //[DebuggerHidden]
            //[DebuggerStepThrough]
            //[DebuggerNonUserCode]
            public object MarshalNativeToManaged(IntPtr pNativeData)
            {
                if (pNativeData == IntPtr.Zero)
                    return null;

                string function = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_failed_function(pNativeData));
                string args = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_failed_args(pNativeData));
                string message = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_error_message(pNativeData));


                //!TODO: custom exception type? 
                var e = new Exception($"{message}{Environment.NewLine}{function}({args})");

                //!TODO: maybe throw only in debug? would need to change all methods to return error\null
                throw e;
                //ThrowIfDebug(e);
                //return e;
            }

            [DebuggerHidden]
            [DebuggerStepThrough]
            [DebuggerNonUserCode]
            [Conditional("DEBUG")]
            private void ThrowIfDebug(Exception e) 
                => throw e;
        }
    }
}