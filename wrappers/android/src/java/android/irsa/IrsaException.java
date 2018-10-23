// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

public class IrsaException extends Exception {

	private int result = 0x00000000;
	private int internalCode = 0;

	public IrsaException(int result) {
		super();
		this.result = result;
	}

	public IrsaException(int result, String message) {
		super(message);
		this.result = result;
	}

	public IrsaException(String message) {
		super(message);

		String resultPart = "Result:";
		int index = message.indexOf(resultPart);
		String errorCodePart = "ErrorCode:";
		int index2 = message.indexOf(errorCodePart);
			
		if (index >=0) {
			try {
				if (index2 >= 0) {
					this.result = Integer.parseInt(message.substring(index+resultPart.length(), index2).trim());
				} else {
					this.result = Integer.parseInt((message.substring(index+resultPart.length())).trim());
				}

			} catch (Exception e) {
			}
		}
			
		if (index2 >=0) {
			try {
				this.internalCode = Integer.parseInt((message.substring(index2+errorCodePart.length())).trim());

			} catch (Exception e) {
			}
		}
	}

	public int getResult() {
		return result;
	}
	
	public int getInternalCode() {
		return internalCode;
	}
}
