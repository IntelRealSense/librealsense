using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FramesReleaser : IDisposable
    {
        public FramesReleaser() { }

        public void AddFrameToRelease<T>(T f) where T : IDisposable
        {
            if (!m_objects.Contains(f))
                m_objects.Add(f);
        }

        // Add an object to a releaser (if one is provided) and return the object
        public static T ScopedReturn<T>(FramesReleaser releaser, T obj) where T : IDisposable
        {
            if (releaser != null) releaser.AddFrameToRelease(obj);
            return obj;
        }

        private HashSet<IDisposable> m_objects = new HashSet<IDisposable>();
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                foreach (var o in m_objects)
                    o.Dispose();
                disposedValue = true;
            }
        }

        ~FramesReleaser()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }

    public static class Helpers
    {
        /// <summary>
        /// Custom marshaler for throwing exceptions on errors codes.
        /// </summary>
        public class ErrorMarshaler : ICustomMarshaler
        {

            //private static ErrorMarshaler Instance = new ErrorMarshaler();
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

                //Debug.LogError("<color=orange>rs_error was raised when calling " + function + "(" + args + ")" + "</color>");
                //Debug.LogError("<color=orange>Message: " + message + "</color>");

                //NativeMethods.rs_free_error(pNativeData);

                var f = String.Format("{0}({1})", function, args);
                //StackTrace stackTrace = new StackTrace(1, true);                
                //Debug.Log(stackTrace.GetFrame(0).GetFileName());

                //!TODO: custom exception type? 
                var e = new Exception(message + Environment.NewLine + f);

                //!TODO: maybe throw only in debug? would need to change all methods to return error\null
                throw e;
                //ThrowIfDebug(e);
                //return e;
            }

            [DebuggerHidden]
            [DebuggerStepThrough]
            [DebuggerNonUserCode]
            [Conditional("DEBUG")]
            void ThrowIfDebug(Exception e)
            {
                throw e;
            }
        }
    }

    public class Log
    {
        public static void ToConsole(LogSeverity severity)
        {
            object err;
            NativeMethods.rs2_log_to_console(severity, out err);
        }

        public static void ToFile(LogSeverity severity, string filename)
        {
            object err;
            NativeMethods.rs2_log_to_file(severity, filename, out err);
        }
    }
}