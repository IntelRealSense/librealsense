// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;


public enum IrsaEventType {

    //keep sync with src/native/irsa_event.h
    IRSA_EVENT_ERROR(100, "IRSA ERROR"),
    IRSA_EVENT_INFO(200, "IRSA INFO");
    //
    //
    public final int IRSA_ERROR_PROBE_RS=6;
    public final int IRSA_ERROR = 100;
    public final int IRSA_INFO  = 200;

	private int type;
	private String message;

	IrsaEventType(int type, String message) {
	    this.type = type;
	    this.message = message;
	}

	public int getValue() {
		return type;
	}

	public static IrsaEventType fromValue(int type) {
		for (IrsaEventType eventType: IrsaEventType.values()) {
			if (eventType.getValue() == type)
				return eventType;
		}
		throw new IllegalArgumentException("Invalid EventType:" + type);
	}

	public String toString() {
		return this.message;
	}
}        
