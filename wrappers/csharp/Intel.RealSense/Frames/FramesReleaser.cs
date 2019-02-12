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
}