using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense
{
    public interface ICompositeDisposable : IDisposable
    {
        void AddDisposable(IDisposable disposable);
    }
}
