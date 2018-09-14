using System;
using System.Collections.Generic;

namespace Intel.RealSense
{
    public class FramesReleaser : IDisposable
    {
        private HashSet<IDisposable> objects;
        private bool disposedValue; // To detect redundant calls

        public FramesReleaser()
        {
            objects = new HashSet<IDisposable>();
            disposedValue = false;
        }

        public void AddFrameToRelease<T>(T f) where T : IDisposable
        {
            if (!objects.Contains(f))
                objects.Add(f);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                foreach (var o in objects)
                    o.Dispose();
                disposedValue = true;
            }
        }

        ~FramesReleaser()
        {
            Dispose(false);
        }

        // Add an object to a releaser (if one is provided) and return the object
        public static T ScopedReturn<T>(FramesReleaser releaser, T obj) where T : IDisposable
        {
            if (releaser != null)
                releaser.AddFrameToRelease(obj);

            return obj;
        }
    }
}