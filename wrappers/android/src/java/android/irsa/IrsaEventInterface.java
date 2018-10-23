// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

abstract class IrsaEventInterface {
    public interface IrsaEventCallback {
        public void onEventInfo(IrsaEventType eventType, int what, int arg1, int arg2, Object obj);
    }
    
}
