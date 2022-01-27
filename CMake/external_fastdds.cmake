if(BUILD_DDS_BACKEND)
    include(ExternalProject)
    
    # Foonathan memory is a dependency of FastDDS.
    ExternalProject_Add(
        foonathan_memory
        PREFIX foonathan_memory
        GIT_REPOSITORY "https://github.com/foonathan/memory.git"
        GIT_TAG "v0.7-1" # Commit Hash: 19ab0759c7f053d88657c0eb86d879493f784d61
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/foonathan_memory
        CMAKE_ARGS -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                   -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                   -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                   -DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}
                   -DCMAKE_C_FLAGS_MINSIZEREL=${CMAKE_C_FLAGS_MINSIZEREL}
                   -DCMAKE_C_FLAGS_RELEASE=${CMAKE_C_FLAGS_RELEASE}
                   -DCMAKE_C_FLAGS_RELWITHDEBINFO=${CMAKE_C_FLAGS_RELWITHDEBINFO}
                   -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
                   -DBUILD_SHARED_LIBS:BOOL=OFF
                   -DFOONATHAN_MEMORY_BUILD_EXAMPLES:BOOL=OFF
                   -DFOONATHAN_MEMORY_BUILD_TESTS:BOOL=OFF
                   -DFOONATHAN_MEMORY_BUILD_TOOLS:BOOL=OFF
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        )
    
    
    # Let FastDDS clone and build it's dependencies (except "Foonathan_memory" which fastdds does not supply as a third party.
    set(FASTDDS_FLAGS   -DBUILD_SHARED_LIBS=OFF 
                        -DTHIRDPARTY_Asio=FORCE 
                        -DTHIRDPARTY_TinyXML2=FORCE 
                        -DTHIRDPARTY_fastcdr=FORCE)
                            
    # We construct the git tag is the purpose of having a single place that indicate the FastDDS version we consume.
    # FastDDS library name is different in Windows/Linux (Windows library name is versioned and Linux is not!)
    #   Windows: libfastrtps-<major-version>.<minor-version>.lib
    #   Linux:   libfastrtps.a 
    # Current used version is "2.5.0"
    # Commit Hash: ecb9711cf2b9bcc608de7d45fc36d3a653d3bf05
    # Git tag "v2.5.0"
    set(FASTDDS_VERSION_MAJOR "2")
    set(FASTDDS_VERSION_MINOR "5")
    set(FASTDDS_VERSION_BUILD "0")
    
    ExternalProject_Add(
        fastdds
        PREFIX fastdds
        GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS.git"
        GIT_TAG "v${FASTDDS_VERSION_MAJOR}.${FASTDDS_VERSION_MINOR}.${FASTDDS_VERSION_BUILD}" 
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/fastdds
        CMAKE_ARGS  -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}
                    -DCMAKE_C_FLAGS_MINSIZEREL=${CMAKE_C_FLAGS_MINSIZEREL}
                    -DCMAKE_C_FLAGS_RELEASE=${CMAKE_C_FLAGS_RELEASE}
                    -DCMAKE_C_FLAGS_RELWITHDEBINFO=${CMAKE_C_FLAGS_RELWITHDEBINFO}
                    -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install
                    -DCMAKE_INSTALL_LIBDIR=lib
                    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                    ${FASTDDS_FLAGS}
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        DEPENDS "foonathan_memory"
    )
    
    set(FASTDDS_DEBUG_TARGET_NAME "libfastrtps")
    set(FASTDDS_RELEASE_TARGET_NAME "libfastrtps")

    if(WIN32)
        set(FASTDDS_DEBUG_TARGET_NAME "${FASTDDS_DEBUG_TARGET_NAME}d-${FASTDDS_VERSION_MAJOR}.${FASTDDS_VERSION_MINOR}")
        set(FASTDDS_RELEASE_TARGET_NAME "${FASTDDS_RELEASE_TARGET_NAME}-${FASTDDS_VERSION_MAJOR}.${FASTDDS_VERSION_MINOR}")
    endif()

    add_library(dds INTERFACE)

    target_include_directories(dds INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/include>)
    target_link_libraries(dds INTERFACE debug ${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/lib/${FASTDDS_DEBUG_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(dds INTERFACE optimized ${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/lib/${FASTDDS_RELEASE_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
endif() #BUILD_DDS_BACKEND
