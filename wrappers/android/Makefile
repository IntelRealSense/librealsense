#used to build entire librealsense & Android wrapper & Android examples

#######################################################
#basepath=$(shell pwd)

#######################################################
#targets= armeabi-v7a x86 arm64-v8a x86_64
targets= armeabi-v7a

#######################################################
#build_type is Release or Debug
#build_type = Release
build_type = Debug

#######################################################
ifneq (, $(findstring ndk-r16b,$(ANDROID_NDK_DIR)))
android_stl=gnustl_static
else
android_stl=c++_static
endif


#######################################################
$(info android_sdk : ${ANDROID_SDK_DIR})
$(info android_ndk : ${ANDROID_NDK_DIR})
$(info android_stl : ${android_stl})
$(info basepath    : ${basepath})

ifeq (, $(ANDROID_SDK_DIR))
$(error "pls run source build/envsetup.sh")
endif

ifeq (, $(ANDROID_NDK_DIR))
$(error "pls run source build/envsetup.sh")
endif


.PHONY: librs clean_librs  \
        irsajni clean_irsajni \
        irsajar clean_irsajar \
        examples clean_examples \
		cleanall

all: prepare librs irsajni irsajar examples
	@echo == Building $@: $^ ==

prepare:
	@echo "nothing need to do"

librs:
	@echo == Building $@: $^ ==
	@for target in ${targets}; do  \
		mkdir -p ${basepath}/out/target/$$target && \
		cd ${basepath}/out/target/$$target && \
		mkdir -p build_librealsense && \
		cd build_librealsense && \
		cmake -DANDROID_ABI=$$target -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_DIR}/build/cmake/android.toolchain.cmake -DBUILD_SHARED_LIBS=false -DANDROID_STL=${android_stl} -DFORCE_LIBUVC=false -DCMAKE_BUILD_TYPE=${build_type} ${librs_src_dir} && \
		make VERBOSE=1 && \
		cd ${basepath} || \
		exit 1; \
	done

clean_librs:
	@echo == Building $@: $^ ==
	@for target in ${targets}; do  \
		rm -rf ${basepath}/out/target/$$target/build_librealsense; \
	done


#to build libirsajni.so we need pass BASEPATH to CMakeLists.txt for irsa
irsajni: librs
	@echo == Building $@: $^ ==
	echo ${basepath}
	@for target in ${targets}; do  \
		mkdir -p ${basepath}/out/target/$$target && \
		cd ${basepath}/out/target/$$target && \
		mkdir -p build_irsajni && \
		cd build_irsajni && \
		cmake -DBASEPATH=${basepath} -DANDROID_ABI=$$target -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_DIR}/build/cmake/android.toolchain.cmake -DBUILD_SHARED_LIBS=true -DANDROID_STL=${android_stl} -DCMAKE_BUILD_TYPE=${build_type} ${basepath} && \
		make VERBOSE=1 && \
		/bin/mkdir -p ${basepath}/examples/irsa_example/app/libs/$$target && \
		/bin/cp -fv ${basepath}/out/target/$$target/build_irsajni/libirsajni.so ${basepath}/examples/irsa_example/app/libs/$$target/ && \
		cd ${basepath} || \
		exit 1; \
	done


clean_irsajni:
	@echo == Building $@: $^ ==
	@for target in ${targets}; do  \
		/bin/rm -rf ${basepath}/out/target/$$target/build_irsajni;  \
	done


irsajar:
	@echo == Building $@: $^ ==
	@cd build && make -f irsajar.mk BASEPATH=${basepath} buildtype=${build_type}  && cd ${basepath}

clean_irsajar:
	@echo == Building $@: $^ ==
	@cd build && make -f irsajar.mk BASEPATH=${basepath} buildtype=${build_type} clean

examples: irsajni irsajar
	make -C examples

clean_examples:
	make -C examples clean

clean: clean_librs  clean_irsajni  clean_irsajar clean_examples
	@/bin/rm -rf out
