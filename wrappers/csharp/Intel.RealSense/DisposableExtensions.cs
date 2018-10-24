using System;
using System.Collections.Generic;
using System.Text;

namespace Intel.RealSense
{
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
}
