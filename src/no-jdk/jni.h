/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

/*
	This file sits in the /src/no-jdk folder, which is intended to be placed AFTER $(JAVA_HOME)/include in the compiler include path.
	Therefore the "#include <jni.h>" statement will reach this file when LibRealsense is compiled on a system with no JDK installed.
	We can still compile LibRealsense and all C/C++/C#/Python examples will work just fine, but we will want to print a message warning
	the user that JNI entrypoints for Java will not be available. If we are producing packaged binaries for distribution, this should
	always be done from a machine WITH the JDK installed. The resulting .so/.dll file will have no runtime dependency on Java, but will
	contain extra entrypoints that allow Java to interact with the library.
*/

#pragma message ("***************************************************************************************************************************")
#pragma message ("* WARNING - No JDK installed or JAVA_HOME not set! JNI entrypoints will not be available in librealsense.so/realsense.dll *")
#pragma message ("***************************************************************************************************************************")

#define NO_JDK