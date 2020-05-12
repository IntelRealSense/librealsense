if(ENABLE_RS_AUTO_UPDATER)
    include(ExternalProject)
    message(STATUS "Building libcurl enabled")
    
    set(CURL_FLAGS -DBUILD_CURL_EXE=OFF -DBUILD_SHARED_LIBS=OFF -DUSE_WIN32_LDAP=OFF  -DCURL_STATIC_CRT=ON -DHTTP_ONLY=ON -DCURL_ZLIB=OFF -DCURL_DISABLE_CRYPTO_AUTH=ON -DCMAKE_USE_OPENSSL=OFF -DCMAKE_USE_LIBSSH2=OFF)

    ExternalProject_Add(
        libcurl
        PREFIX libcurl
        GIT_REPOSITORY "https://github.com/curl/curl.git"
        GIT_TAG "53cdc2c963e33bc0cc1a51ad2df79396202e07f8" # curl-7_70_0
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/libcurl
        TEST_COMMAND ""
        CMAKE_ARGS  -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install
                    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DANDROID_ABI=${ANDROID_ABI}
                    -DANDROID_STL=${ANDROID_STL} ${CURL_FLAGS}
                    
    )
    if (WIN32)    # This names only covers STATIC LIB build for now
        set(CURL_TARGET_NAME "libcurl-d")
    else()
        set(CURL_TARGET_NAME "libcurl")
    endif()

    add_library(curl INTERFACE)
    target_include_directories(curl INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install/include>)
    target_link_libraries(curl INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install/lib/${CURL_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
endif() #ENABLE_RS_AUTO_UPDATER
