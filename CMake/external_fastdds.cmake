if(BUILD_DDS_BACKEND)
    include(ExternalProject)
    message(STATUS "Building fastdds enabled")
    
    # Foonathan memory.
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
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/foonathan_memory/foonathan_memory_install
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
        )
    
    set(FASTDDS_FLAGS -DBUILD_SHARED_LIBS=OFF -DTHIRDPARTY_Asio=FORCE -DTHIRDPARTY_TinyXML2=FORCE -DTHIRDPARTY_fastcdr=FORCE)
    set(FASTDDS_FLAGS ${FASTDDS_FLAGS} -Dfoonathan_memory_DIR=${CMAKE_CURRENT_BINARY_DIR}/foonathan_memory/foonathan_memory_install/share/foonathan_memory/cmake)
    
    ExternalProject_Add(
        fastdds
        PREFIX fastdds
        GIT_REPOSITORY "https://github.com/eProsima/Fast-DDS.git"
        GIT_TAG "v2.5.0" # Commit Hash: ecb9711cf2b9bcc608de7d45fc36d3a653d3bf05
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
    )

    #set(FASTDDS_DEBUG_TARGET_NAME "libfastrtpsd")
    #set(FASTDDS_RELEASE_TARGET_NAME "libfastrtps")
    
    #set(FASTCDR_DEBUG_TARGET_NAME "libfastcdrd")
    #set(FASTCDR_RELEASE_TARGET_NAME "libfastcdr")
    add_library(dds INTERFACE)

    target_include_directories(dds INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/include>)
    #message(STATUS "fastdds_LIBRARIES = ${fastdds_LIBRARIES}  ")
    #target_link_libraries(dds INTERFACE debug ${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/lib/${FASTDDS_DEBUG_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    #target_link_libraries(dds INTERFACE optimized ${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/lib/${FASTDDS_RELEASE_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    link_directories(${CMAKE_CURRENT_BINARY_DIR}/fastdds/fastdds_install/lib)
endif() #BUILD_DDS_BACKEND
