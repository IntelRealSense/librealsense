using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public class FramesReleaser : ICompositeDisposable
    {
        internal readonly List<IDisposable> disposables = new List<IDisposable>();

        public void AddDisposable(IDisposable disposable)
        {
            disposables.Add(disposable);
        }

        [Obsolete("This method is obsolete. Use AddDisposable method instead")]
        // Add an object to a releaser (if one is provided) and return the object
        public static T ScopedReturn<T>(FramesReleaser releaser, T obj) where T : IDisposable
        {
            if (releaser != null) releaser.AddDisposable(obj);
            return obj;
        }

        #region IDisposable Support
        internal bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.

                disposables.ForEach(d => d?.Dispose());

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~FramesReleaser()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion
    }

    public struct StreamComparer : IEqualityComparer<Stream>
    {
        public static readonly StreamComparer Default = new StreamComparer();

        public bool Equals(Stream x, Stream y)
        {
            return x == y;
        }

        public int GetHashCode(Stream obj)
        {
            // you need to do some thinking here,
            return (int)obj;
        }
    }

    public interface ICompositeDisposable : IDisposable
    {
        void AddDisposable(IDisposable disposable);
    }

    // https://leeoades.wordpress.com/2012/08/29/neat-disposal-pattern/
    public static class DisposableExtensions
    {
        public static T DisposeWith<T>(this T disposable, ICompositeDisposable composite) where T : IDisposable
        {
            if (disposable == null || composite == null)
                return disposable;
            composite.AddDisposable(disposable);
            return disposable;
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