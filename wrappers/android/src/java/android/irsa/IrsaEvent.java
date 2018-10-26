// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;


final public class IrsaEvent {

    //keep sync with src/native/irsa_event.h
    public final static int IRSA_ERROR = 100;
    public final static int IRSA_INFO  = 200;

    public final static int IRSA_INFO_PREVIEW_START = 0;
    public final static int IRSA_INFO_PREVIEW_STOP  = 1;
    public final static int IRSA_INFO_SETPREVIEWDISPLAY = 2;
    public final static int IRSA_INFO_SETRENDERID = 3;
    public final static int IRSA_INFO_OPEN = 4;
    public final static int IRSA_INFO_CLOSE = 5;
    public final static int IRSA_INFO_FA = 6;
    public final static int IRSA_INFO_PROBE_RS = 7;
    public final static int IRSA_INFO_PREVIEW = 8;

    public final static int IRSA_ERROR_PREVIEW_START = 0;
    public final static int IRSA_ERROR_PREVIEW_STOP  = 1;
    public final static int IRSA_ERROR_SETPREVIEWDISPLAY = 2;
    public final static int IRSA_ERROR_SETRENDERID = 3;
    public final static int IRSA_ERROR_OPEN = 4;
    public final static int IRSA_ERROR_CLOSE = 5;
    public final static int IRSA_ERROR_FA = 6;
    public final static int IRSA_ERROR_PROBE_RS = 7;
    public final static int IRSA_ERROR_PREVIEW = 8;
}
