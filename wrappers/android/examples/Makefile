#used to build all examples
sub_dir= irsa_example

all:
	@for dir in ${sub_dir}; do \
		cd $$dir && \
		rm -f local.properties && \
		echo "ndk.dir=${ANDROID_NDK_DIR}" >> local.properties && \
		echo "sdk.dir=${ANDROID_SDK_DIR}" >> local.properties && \
		gradle build && \
		cd .. || \
		exit 1; \
	done

clean:
	@for dir in ${sub_dir}; do \
		cd $$dir && \
		rm -f local.properties && \
		echo "ndk.dir=${ANDROID_NDK_DIR}" >> local.properties && \
		echo "sdk.dir=${ANDROID_SDK_DIR}" >> local.properties && \
		gradle clean && \
		cd .. || \
		exit 1; \
	done
	/bin/rm -rf ./irsa_example/app/libs

