cmake .. -DANDROID_ABI=armeabi-v7a -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk-r16b/build/cmake/android.toolchain.cmake -DBUILD_ANDROI=true -DFORCE_LIBUVC=false -DBUILD_SHARED_LIBS=false

make 
