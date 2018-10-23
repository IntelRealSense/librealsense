// License: Apache 2.0. See LICENSE file in root directory.
//
package android.irsa;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Observable;

import android.os.Build;

class IrsaLoader {
    private static final String TAG = IrsaLoader.class.getSimpleName();
    private static final String JNILIB_NAME = "irsajni";
    private static Boolean sLoaded = new Boolean(false);
    private static String sLoadedLibrarySuffix = "";
	
    private IrsaLoader() {}

    public static boolean hasLoaded() { return sLoaded; }

    public static void load() throws UnsatisfiedLinkError {
        synchronized (sLoaded) {
            if (!sLoaded) {
                load(JNILIB_NAME);
                sLoaded = true;
            }
        }
    }

    public static void load(String libName) throws UnsatisfiedLinkError {
        if (!loadJNI(libName)) {
            throw new UnsatisfiedLinkError();
        }
    }

    public static String getLoadedLibrarySuffix() {
        return sLoadedLibrarySuffix;
    }
	
    private static String getCPUInfoField( String cpuInfo, String field_name )
    {
        if (cpuInfo == null || field_name == null) {
            return null;
        }

		String findStr = "\n" + field_name + "\t: ";
		int stringStart = cpuInfo.indexOf(findStr);
		if (stringStart < 0) {
			findStr = "\n" + field_name + ": ";
			stringStart = cpuInfo.indexOf(findStr);
			if (stringStart < 0) {
				return null;
            }
		}
		int start = stringStart + findStr.length();
		int end = cpuInfo.indexOf("\n", start);

		return cpuInfo.substring(start, end);
    }

    private static String[] GetCPUSuffixes() {
		String cpuInfo = readCPUInfo();
		String cpuArchitecture = getCPUInfoField(cpuInfo,"CPU architecture");
		String cpuFeature = getCPUInfoField(cpuInfo, "Features");
		String vendorId = getCPUInfoField(cpuInfo, "vendor_id");

		String CPU_1 = Build.CPU_ABI;
		String CPU_2 = Build.CPU_ABI2;

		IrsaLog.d(TAG, "cpuinfo:\n " +  cpuInfo);
		IrsaLog.d(TAG, "cpuArch  :" + cpuArchitecture);
		IrsaLog.d(TAG, "cpuFeature  :" + cpuFeature);
		IrsaLog.d(TAG, "CPU_ABI  : " + CPU_1);
		IrsaLog.d(TAG, "CPU_ABI2 : " + CPU_2);

		boolean isX86 = false;
		String[] cpus = null;

		if ( vendorId != null && 
			(CPU_1.equals("x86") || CPU_2.equals("x86")) && 
			vendorId.equals("GenuineIntel") ) {
			cpus = new String[] {"x86"}; 
			isX86 = true;
		}
		
		if (!isX86 && cpuArchitecture != null) {
			if (cpuArchitecture.startsWith("7") || cpuArchitecture.startsWith("8")) {
				if (cpuFeature != null && cpuFeature.contains("neon")) {
					cpus = new String[] {"arm64-v8a", "armv8", "armeabi-v7a", "armv7"};
                } else if (cpuArchitecture.startsWith("8")) {
					cpus = new String[] {"arm64-v8a", "armv8"};
				}
	 		} 
		}

	 	return cpus;
    }

    private static String readCPUInfo() {
		String result = " ";
		try {
		    if (new File("/proc/cpuinfo").exists()) {
				BufferedReader brCpuInfo = new BufferedReader(new FileReader(new File("/proc/cpuinfo")));
	            String aLine;

	            if (brCpuInfo != null) {
					while ((aLine = brCpuInfo.readLine()) != null) {
						result = result + aLine + "\n";
					}
					brCpuInfo.close();
	            }
    		}
		} catch (Exception ex) {
			ex.printStackTrace();
		}

		return result;
    }

    private static boolean loadJNI(String lib) {
        boolean loaded = false;

        String[] cpus = GetCPUSuffixes();
        List<String> list = new ArrayList<String>();

        for (String cpu: cpus) {
            if (android.os.Build.VERSION.SDK_INT < 16 ) {
                list.add(lib +  "_" + cpu);
            } else {
                list.add(lib +  "_" + cpu);
            }
        }

        list.add(lib);

        for (String temp : list) {
            IrsaLog.d(TAG, "lib : " + temp);
        }

        for (String str : list) {
            try {
            	IrsaLog.d(TAG, "try to loadLibrary " + str);
                System.loadLibrary(str);
                sLoadedLibrarySuffix = str.substring(lib.length());
            	IrsaLog.d(TAG, "loadLibrary " + str + " succeed");
                loaded = true;
                break;
            } catch (UnsatisfiedLinkError ule) {
                IrsaLog.d(TAG, "can't load " + str + ", continue to next ...");
            }
        }

        return loaded;
    }
}
