"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" librealsense.vc12\realsense\realsense.vcxproj /p:Configuration=Release;Platform=x64

rm /S /Q librealsense_x64-dll.zip
rmdir /S /Q librealsense-dll

mkdir librealsense-dll
mkdir librealsense-dll\bin
mkdir librealsense-dll\include
mkdir librealsense-dll\examples

xcopy bin\x64\* librealsense-dll\bin /S
xcopy include\* librealsense-dll\include /S
xcopy examples\* librealsense-dll\examples /S

powershell.exe -nologo -noprofile -command "& { $theDate = (Get-Date); $theDate.ToShortDateString() | Out-File -FilePath librealsense-dll\release.txt -Append; \"https://github.intel.com/PerCSystemsEngineering/librealsense/\" | Out-File -FilePath librealsense-dll\release.txt -Append;}"
powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::CreateFromDirectory('librealsense-dll', 'librealsense_x64-dll.zip'); }"

rmdir /S /Q librealsense-dll