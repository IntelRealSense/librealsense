"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Release;Platform=x64
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Release;Platform=Win32
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Debug;Platform=x64
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Debug;Platform=Win32

rm /S /Q librealsense-dll.zip
rmdir /S /Q librealsense-dll

mkdir librealsense-dll
mkdir librealsense-dll\bin
mkdir librealsense-dll\include
mkdir librealsense-dll\bindings
mkdir librealsense-dll\examples
mkdir librealsense-dll\librealsense.vc12

xcopy bin\x64\* librealsense-dll\bin /S
xcopy include\* librealsense-dll\include /S
xcopy bindings\* librealsense-dll\bindings /S
xcopy examples\* librealsense-dll\examples /S
xcopy librealsense.vc12\* librealsense-dll\librealsense.vc12 /S
xcopy COPYING librealsense-dll\
xcopy readme.md librealsense-dll\

rmdir /S /Q librealsense-dll\librealsense.vc12\realsense
del librealsense-dll\librealsense.vc12\realsense.sln
del librealsense-dll\librealsense.vc12\.gitignore

powershell.exe -nologo -noprofile -command "& { $theDate = (Get-Date); $theDate.ToShortDateString() | Out-File -FilePath librealsense-dll\release.txt -Append; \"https://github.intel.com/PerCSystemsEngineering/librealsense/\" | Out-File -FilePath librealsense-dll\release.txt -Append;}"
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::CreateFromDirectory('librealsense-dll', 'librealsense-dll.zip'); }"

rmdir /S /Q librealsense-dll