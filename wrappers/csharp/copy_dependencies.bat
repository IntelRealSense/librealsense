echo Configuration = %2
echo Platform = %1

if not exist "%3..\..\build\%2" goto nolocal
echo Copying from local compilation
xcopy /Y "%3..\..\build\%2\realsense2.dll" %4
exit 0
:nolocal
if not exist "C:\Program Files (x86)\Intel RealSense SDK 2.0" goto noinstall
echo Copying from installed SDK
xcopy /Y "C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\%1\realsense2.dll" %4
exit 0
:noinstall
echo "realsense2.dll NOT FOUND! Please build it from source using CMake or install Intel RealSense SDK 2.0 from GitHub releases"
exit 1