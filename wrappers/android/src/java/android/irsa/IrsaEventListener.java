// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

public interface IrsaEventListener {    
	public void onEvent(
			IrsaEventType eventType,
			int what, 
			int arg1, 
			int arg2, 
			Object obj);     	 
}
