How to build entire Android wrapper:

The Host dev OS is Ubuntu 18.04.1 LTS (validation passed with Ubuntu 18.04.1 x86-64 LTS , should 
work fine with Ubuntu 14 or Ubuntu 16)

1. Prerequisites:

running build/prepare.sh to setup devlopment envs for IRSA:

install jdk-8u112-linux-x64.tar.gz 
install gradle-4.6-all.zip 
install android-ndk-r16b-linux-x86_64.zip
install android-sdk-ndk-from-AS3.2.0.xz

install cmake 
install git   

$ sudo build/prepare.sh

2. build Android wrapper

 modify build/envsetup.sh accordingly:

 ATTENTION: there are two C++ STL libraries in Android NDK,

 using libc++ (c++_static) in android-sdk-ndk-from-AS3.2.0.xz(NDK in Android Studio 3.2.0 
 which NDK's Pkg.Revision = 18.0.5002713;

 using libstdc++ (gnustl_static) with android-ndk-r16b;

 which library should to used in IRSA:
      (1) Google's NDK 18.0.5002713 already remove libstdc++(GNU c++ library) officially
      (2) select android-ndk-r16b if your proprietary/business libraries was built with
          libstdc++(gnustl) 
      (3) select android-ndk-r16b or NDK in android-sdk-ndk-from-AS3.2.0.xz
          if your proprietary/business libraries was built with libc++(cxxstl);
      (4) pls keep sync between ANDROID_STL setting in examples/rs_multicam (line 20 in 
          example/rs_multicam/app/build.gradle) and NDK selection in
          build/envsetup;

    
 modify targets or build_type in Makefile accordingly

 $ source build/envsetup.sh

 (send build_android_wrapper_log.txt to us if issues occured)

 $ make 2>&1 | tee build_android_wrapper_log.txt   

 all build intermediates and libs or jars will be found at directory "output".
