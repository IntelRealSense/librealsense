ECHO OFF

SET SRC_DIR=%~dp0
SET LIBREALSENSE_DIR=%SRC_DIR%\..\..

IF "x%ANDROID_SDK%x"=="xx" (
    SET ANDROID_SDK=C:\Yu\Applications\android-sdk
    ECHO "Please make sure Android SDK/CMake/NDK are installed in Android Studio!"
    ECHO "And Android SDK is stored in %ANDROID_SDK"
    ECHO ANDROID_SDK     : %ANDROID_SDK%

    PAUSE
)

PUSHD %LIBREALSENSE_DIR%

SET ANDROID_ABI=armeabi-v7a
ECHO "Building %ANDROID_ABI%"
MKDIR build-%ANDROID_ABI%
PUSHD build-%ANDROID_ABI%
%ANDROID_SDK%\cmake\3.6.4111459\bin\cmake.exe -GNinja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ANDROID_SDK%\ndk-bundle\build\cmake\android.toolchain.cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe -DANDROID_ABI:STRING=%ANDROID_ABI% -DANDROID_STL:STRING=c++_static -DUSE_SYSTEM_LIBUSB:BOOL=OFF -DFORCE_LIBUVC:BOOL=OFF -DBUILD_GRAPHICAL_EXAMPLES:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release ..
%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe
XCOPY /y librealsense2.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y libusb_install\lib\libusb.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y third-party\realsense-file\librealsense-file.a %SRC_DIR%\libs\%ANDROID_ABI%\
POPD

SET ANDROID_ABI=arm64-v8a
ECHO "Building %ANDROID_ABI%"
MKDIR build-%ANDROID_ABI%
PUSHD build-%ANDROID_ABI%
%ANDROID_SDK%\cmake\3.6.4111459\bin\cmake.exe -GNinja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ANDROID_SDK%\ndk-bundle\build\cmake\android.toolchain.cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe -DANDROID_ABI:STRING=%ANDROID_ABI% -DANDROID_STL:STRING=c++_static -DUSE_SYSTEM_LIBUSB:BOOL=OFF -DFORCE_LIBUVC:BOOL=OFF -DBUILD_GRAPHICAL_EXAMPLES:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release ..
%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe
XCOPY /y librealsense2.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y libusb_install\lib\libusb.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y third-party\realsense-file\librealsense-file.a %SRC_DIR%\libs\%ANDROID_ABI%\
POPD

SET ANDROID_ABI=x86
ECHO "Building %ANDROID_ABI%"
MKDIR build-%ANDROID_ABI%
PUSHD build-%ANDROID_ABI%
%ANDROID_SDK%\cmake\3.6.4111459\bin\cmake.exe -GNinja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ANDROID_SDK%\ndk-bundle\build\cmake\android.toolchain.cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe -DANDROID_ABI:STRING=%ANDROID_ABI% -DANDROID_STL:STRING=c++_static -DUSE_SYSTEM_LIBUSB:BOOL=OFF -DFORCE_LIBUVC:BOOL=OFF -DBUILD_GRAPHICAL_EXAMPLES:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release ..
%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe
XCOPY /y librealsense2.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y libusb_install\lib\libusb.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y third-party\realsense-file\librealsense-file.a %SRC_DIR%\libs\%ANDROID_ABI%\
POPD

SET ANDROID_ABI=x86_64
ECHO "Building %ANDROID_ABI%"
MKDIR build-%ANDROID_ABI%
PUSHD build-%ANDROID_ABI%
%ANDROID_SDK%\cmake\3.6.4111459\bin\cmake.exe -GNinja -DCMAKE_TOOLCHAIN_FILE:FILEPATH=%ANDROID_SDK%\ndk-bundle\build\cmake\android.toolchain.cmake -DCMAKE_MAKE_PROGRAM:FILEPATH=%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe -DANDROID_ABI:STRING=%ANDROID_ABI% -DANDROID_STL:STRING=c++_static -DUSE_SYSTEM_LIBUSB:BOOL=OFF -DFORCE_LIBUVC:BOOL=OFF -DBUILD_GRAPHICAL_EXAMPLES:BOOL=OFF -DBUILD_SHARED_LIBS:BOOL=OFF -DCMAKE_BUILD_TYPE:STRING=Release ..
%ANDROID_SDK%\cmake\3.6.4111459\bin\ninja.exe
XCOPY /y librealsense2.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y libusb_install\lib\libusb.a %SRC_DIR%\libs\%ANDROID_ABI%\
XCOPY /y third-party\realsense-file\librealsense-file.a %SRC_DIR%\libs\%ANDROID_ABI%\
POPD

ECHO "Done."

POPD

:END
