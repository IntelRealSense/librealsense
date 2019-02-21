using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    /// <summary>
    /// Custom marshaler for throwing exceptions on errors codes.
    /// </summary>
    public class ErrorMarshaler : ICustomMarshaler
    {
        private static ErrorMarshaler Instance;

        public static ICustomMarshaler GetInstance(string s)
        {
            if (Instance == null)
            {
                Instance = new ErrorMarshaler();
            }
            return Instance;
        }

        public void CleanUpManagedData(object ManagedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            //!TODO: maybe rs_free_error here?
            NativeMethods.rs2_free_error(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return IntPtr.Size;
        }

        public IntPtr MarshalManagedToNative(object ManagedObj)
        {
            return IntPtr.Zero;
        }

        public object MarshalNativeToManaged(IntPtr pNativeData)
        {
            if (pNativeData == IntPtr.Zero)
                return null;

            string function = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_failed_function(pNativeData));
            string args = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_failed_args(pNativeData));
            string message = Marshal.PtrToStringAnsi(NativeMethods.rs2_get_error_message(pNativeData));
            ExceptionType type = NativeMethods.rs2_get_librealsense_exception_type(pNativeData);

            var inner = new ExternalException($"{function}({args})");
            switch (type)
            {
                case ExceptionType.NotImplemented:
                throw new NotImplementedException(message, inner);

            case ExceptionType.WrongApiCallSequence:
                throw new InvalidOperationException(message, inner);

            case ExceptionType.InvalidValue:
                throw new ArgumentException(message, inner);

            case ExceptionType.Io:
                throw new System.IO.IOException(message, inner);

            default:
                throw new Exception(message, inner);
            }
        }
    }
}
