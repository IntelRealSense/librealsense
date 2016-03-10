# Linking

## Dynamic
Build the dynamic library by issuing

    make library

Link binaries simply with `-lrealsense`.

You might need to add a `-Llib`  flag, where `lib` is the directory where the .so file resides, in order for the linker to find the library.

## Static
Build the static library by issuing

    make lib/librealsense.a
	
Binaries needs to be linked with this library and also `-lusb-1.0` and `-pthread`
