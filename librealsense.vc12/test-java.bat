"C:\Program Files (x86)\MSBuild\12.0\Bin\MSBuild.exe" realsense\realsense.vcxproj /p:Configuration=Release;Platform=x64
"%JAVA_HOME%\bin\javac.exe" -classpath ..\examples;..\bindings ..\examples\java_enumerate.java
"%JAVA_HOME%\bin\java.exe" -classpath ..\examples;..\bindings -Djava.library.path=../bin/x64 java_enumerate
