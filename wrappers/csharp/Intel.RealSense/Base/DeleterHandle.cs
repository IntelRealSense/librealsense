// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense.Base
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Security;

    [SuppressUnmanagedCodeSecurity]
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Deleter(IntPtr ptr);

    /// <summary>
    /// Native handle with deleter delegate to release unmanaged resources
    /// </summary>
    // TODO: CriticalFinalizerObject & CER
    //[DebuggerDisplay("{deleter?.Method.Name,nq}", Name = nameof(Deleter))]
    internal sealed class DeleterHandle : IDisposable
    {
        private IntPtr handle;
        private Deleter deleter;

        public IntPtr Handle => handle;

        /// <summary>
        /// Gets a value indicating whether this handle is invalid
        /// </summary>
        public bool IsInvalid => handle == IntPtr.Zero;

        public DeleterHandle(IntPtr ptr, Deleter deleter)
        {
            handle = ptr;
            this.deleter = deleter;
        }

        public void SetHandleAsInvalid()
        {
            handle = IntPtr.Zero;
            GC.SuppressFinalize(this);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public void Dispose(bool disposing)
        {
            if (handle == IntPtr.Zero)
            {
                return;
            }

            deleter?.Invoke(handle);
            handle = IntPtr.Zero;
        }

        internal void Reset(IntPtr ptr)
        {
            handle = ptr;
            GC.ReRegisterForFinalize(this);
        }

        internal void Reset(IntPtr ptr, Deleter deleter)
        {
            this.handle = ptr;
            this.deleter = deleter;
            //GC.ReRegisterForFinalize(this);
        }

        ~DeleterHandle()
        {
            //Console.WriteLine($"~{handle} {deleter?.Method}");
            Dispose(false);
        }
    }
}