"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Release;Platform=x64
"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Release;Platform=Win32

rm /S /Q librealsense-dll.zip
rmdir /S /Q librealsense-dll

mkdir librealsense-dll
mkdir librealsense-dll\bin
mkdir librealsense-dll\include
mkdir librealsense-dll\bindings
mkdir librealsense-dll\examples
mkdir librealsense-dll\librealsense.vc12

xcopy bin\* librealsense-dll\bin /S
xcopy include\* librealsense-dll\include /S
xcopy bindings\* librealsense-dll\bindings /S
xcopy examples\* librealsense-dll\examples /S
xcopy librealsense.vc12\* librealsense-dll\librealsense.vc12 /S
xcopy COPYING librealsense-dll\
xcopy readme.md librealsense-dll\

del librealsense-dll\bin\Win32\*.exp
del librealsense-dll\bin\Win32\*.pdb
del librealsense-dll\bin\x64\*.exp
del librealsense-dll\bin\x64\*.pdb

echo F | xcopy librealsense-dll\bin\Win32\realsense.dll librealsense-dll\bin\Win32\realsense-d.dll
echo F | xcopy librealsense-dll\bin\Win32\realsense.lib librealsense-dll\bin\Win32\realsense-d.lib
echo F | xcopy librealsense-dll\bin\x64\realsense.dll librealsense-dll\bin\x64\realsense-d.dll
echo F | xcopy librealsense-dll\bin\x64\realsense.lib librealsense-dll\bin\x64\realsense-d.lib

del librealsense-dll\librealsense.vc12\realsense.sln
del librealsense-dll\librealsense.vc12\.gitignore
rmdir /S /Q librealsense-dll\librealsense.vc12\realsense

powershell.exe -nologo -noprofile -command "& { $theDate = (Get-Date); $theDate.ToShortDateString() | Out-File -FilePath librealsense-dll\release.txt -Append; \"https://github.intel.com/PerCSystemsEngineering/librealsense/\" | Out-File -FilePath librealsense-dll\release.txt -Append;}"
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::CreateFromDirectory('librealsense-dll', 'librealsense-dll.zip'); }"

rmdir /S /Q librealsense-dll