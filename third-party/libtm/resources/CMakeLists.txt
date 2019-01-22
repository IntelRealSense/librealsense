cmake_minimum_required(VERSION 2.8)
project(resources)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(FW_ARTIFACTORY_DIR "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/target")
set(CENTRAL_APP_ARTIFACTORY_DIR "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/app")
set(CENTRAL_BL_ARTIFACTORY_DIR "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/bl")

set(FW_OUTPUT_FILE "${LIBTM_SRC_DIR}/fw.h")
set(CENTRAL_APP_OUTPUT_FILE "${LIBTM_SRC_DIR}/CentralAppFw.h")
set(CENTRAL_BL_OUTPUT_FILE "${LIBTM_SRC_DIR}/CentralBlFw.h")

# Convert HEX to DEC
function(from_hex HEX DEC)
    string(SUBSTRING "${HEX}" 0 -1 HEX)
    string(TOUPPER "${HEX}" HEX)
    set(_res 0)
    string(LENGTH "${HEX}" _strlen)

    while(_strlen GREATER 0)
        math(EXPR _res "${_res} * 16")
        string(SUBSTRING "${HEX}" 0 1 NIBBLE)
        string(SUBSTRING "${HEX}" 1 -1 HEX)
        if(NIBBLE STREQUAL "A")
            math(EXPR _res "${_res} + 10")
        elseif(NIBBLE STREQUAL "B")
            math(EXPR _res "${_res} + 11")
        elseif(NIBBLE STREQUAL "C")
            math(EXPR _res "${_res} + 12")
        elseif(NIBBLE STREQUAL "D")
            math(EXPR _res "${_res} + 13")
        elseif(NIBBLE STREQUAL "E")
            math(EXPR _res "${_res} + 14")
        elseif(NIBBLE STREQUAL "F")
            math(EXPR _res "${_res} + 15")
        else()
            math(EXPR _res "${_res} + ${NIBBLE}")
        endif()

        string(LENGTH "${HEX}" _strlen)
    endwhile()
    
    set(${DEC} ${_res} PARENT_SCOPE)
endfunction()

# Creates hex buffer from binary file - binary file must be 4 bytes aligned
function(readBinHeaderField input_bin_file offset limit output_hex_buffer)
        #message(STATUS "Reading BIN header from ${input_bin_file} offset ${offset} limit ${limit}")

        file(READ ${input_bin_file} buffer LIMIT ${limit} OFFSET ${offset} HEX)
        string(TOUPPER "${buffer}" buffer)
        string(REGEX REPLACE "([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])" "\\4\\3\\2\\1" buffer "${buffer}")
        from_hex("${buffer}" buffer)

        # Copy ready buffer to output buffer
        set(${output_hex_buffer} "${buffer}" PARENT_SCOPE)
endfunction()

# Creates hex buffer from binary file - binary file must be 4 bytes aligned
function(bin2h input_bin_file offset output_hex_buffer)
        # message(STATUS "Creating HEX buffer from ${input_bin_file} offset ${offset}")

        # Read hex data from file
        file(READ ${input_bin_file} buffer OFFSET ${offset} HEX)

        # Move all buffer to upper case
        string(TOUPPER "${buffer}" buffer)

        # Convert every 4 bytes from AABBCCDD to 0xDDCCBBAA
        string(REGEX REPLACE "([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])([0-9A-F][0-9A-F])" "0x\\4\\3\\2\\1," buffer "${buffer}")

        # Add new line to every 16 columns
        string(REGEX REPLACE "(0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,0x........,)" "\\1\\n" buffer "${buffer}")

        # Copy ready buffer to output buffer
        set(${output_hex_buffer} "${buffer}" PARENT_SCOPE)
endfunction()


if(FW_VERSION)
    message(STATUS "--------------------------------------------------------------------------------------------------------------------------------------------------------------")

    if (FW_SOURCE MATCHES "Remote")
        file(REMOVE ${LIBTM_RESOURCES_DIR}/target.mvcmd)
        message(STATUS "Downloading FW ${FW_VERSION} from '${FW_ARTIFACTORY_DIR}/${FW_VERSION}/target-${FW_VERSION}.mvcmd'")
        file(DOWNLOAD "${FW_ARTIFACTORY_DIR}/${FW_VERSION}/target-${FW_VERSION}.mvcmd" "${LIBTM_RESOURCES_DIR}/target.mvcmd" TIMEOUT 60 STATUS status LOG log)
        # message(STATUS "Downloading FW ${FW_VERSION} completed - status ${status}")

        list (FIND status "\"No error\"" _index)
        if (${_index} EQUAL -1)
            message(FATAL_ERROR "Download error ${FW_ARTIFACTORY_DIR}/${FW_VERSION}/target-${FW_VERSION}.mvcmd" )
        endif()
    elseif (FW_SOURCE MATCHES "Local")
        if(NOT EXISTS "${LIBTM_RESOURCES_DIR}/target.mvcmd")
            message(FATAL_ERROR "File ${LIBTM_RESOURCES_DIR}/target.mvcmd is missing" )
        else()
            message(STATUS "Downloading FW ${FW_VERSION} skipped")
        endif()
    endif()

    if (FW_SOURCE MATCHES "Remote" OR FW_SOURCE MATCHES "Local")
        file(REMOVE ${FW_OUTPUT_FILE})
        message(STATUS "Converting FW version ${FW_VERSION} from target.mvcmd to ${FW_OUTPUT_FILE}")

        # Create empty output file
        file(WRITE ${FW_OUTPUT_FILE} "")
        file(APPEND ${FW_OUTPUT_FILE} "/*******************************************************************************\n")
        file(APPEND ${FW_OUTPUT_FILE} "INTEL CORPORATION PROPRIETARY INFORMATION\n")
        file(APPEND ${FW_OUTPUT_FILE} "Copyright(c) 2017 Intel Corporation. All Rights Reserved.\n")
        file(APPEND ${FW_OUTPUT_FILE} "*******************************************************************************/\n\n")
        file(APPEND ${FW_OUTPUT_FILE} "#ifndef target_h\n")
        file(APPEND ${FW_OUTPUT_FILE} "#define target_h\n\n")
        file(APPEND ${FW_OUTPUT_FILE} "#define FW_VERSION \"${FW_VERSION}\"\n\n")
        file(APPEND ${FW_OUTPUT_FILE} "static uint32_t target_hex []  = {\n")
    
        bin2h(${LIBTM_RESOURCES_DIR}/target.mvcmd 0 fw_ready_buffer)

        file(APPEND ${FW_OUTPUT_FILE} "${fw_ready_buffer}\n")
        file(APPEND ${FW_OUTPUT_FILE} "};\n")
        file(APPEND ${FW_OUTPUT_FILE} "#endif\n")
    else()
        if (NOT EXISTS "${FW_OUTPUT_FILE}")
            message(FATAL_ERROR "File ${FW_OUTPUT_FILE} is missing" )
        else()
            message(STATUS "Already created ${FW_OUTPUT_FILE}")
        endif()
    endif()
endif(FW_VERSION) 


if(CENTRAL_APP_VERSION)
    # message(STATUS "--------------------------------------------------------------------------------------------------------------------------------------------------------------")

    if (FW_SOURCE MATCHES "Remote")
        file(REMOVE ${LIBTM_RESOURCES_DIR}/central_app.bin)
        message(STATUS "Downloading Central App ${CENTRAL_APP_VERSION} from '${CENTRAL_APP_ARTIFACTORY_DIR}/${CENTRAL_APP_VERSION}/central_app-${CENTRAL_APP_VERSION}.bin'")   
        file(DOWNLOAD "${CENTRAL_APP_ARTIFACTORY_DIR}/${CENTRAL_APP_VERSION}/central_app-${CENTRAL_APP_VERSION}.bin" "${LIBTM_RESOURCES_DIR}/central_app.bin" TIMEOUT 60 STATUS status LOG log)
        # message(STATUS "Downloading Central App ${CENTRAL_APP_VERSION} completed - status ${status}")
        list (FIND status "\"No error\"" _index)
        if (${_index} EQUAL -1)
            message(FATAL_ERROR "Download error ${CENTRAL_APP_ARTIFACTORY_DIR}/${CENTRAL_APP_VERSION}/central_app-${CENTRAL_APP_VERSION}.bin" )
        endif()
    elseif (FW_SOURCE MATCHES "Local")
        if(NOT EXISTS "${LIBTM_RESOURCES_DIR}/central_app.bin")
            message(FATAL_ERROR "File ${LIBTM_RESOURCES_DIR}/central_app.bin is missing" )
        else()
            message(STATUS "Downloading Central App ${CENTRAL_APP_VERSION} skipped")
        endif()
    endif()
        
    if (FW_SOURCE MATCHES "Remote" OR FW_SOURCE MATCHES "Local")
        file(REMOVE ${CENTRAL_APP_OUTPUT_FILE})
        message(STATUS "Converting Central App version ${CENTRAL_APP_VERSION} from central_app.bin to ${CENTRAL_APP_OUTPUT_FILE}")

        STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+\\.[0-9]+" "\\1" central_app_major_version "${CENTRAL_APP_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" central_app_minor_version "${CENTRAL_APP_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" central_app_patch_version "${CENTRAL_APP_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" central_app_build_version "${CENTRAL_APP_VERSION}")

        # Central APP Header and data format:
        # Header:
        #        uint32 header_size;
        #        uint32 data_size;
        #         uint32 file_format_version; // defines the format of both header and data
        #         uint32 version_size;
        #         uint8 version[]
        # Data: 
        #         uint8 data[];

        # Read 1 byte of file_format_version and abort if not supported
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 8 1 central_app_format_ver)
        if (NOT ${central_app_format_ver} EQUAL 1)
            message(FATAL_ERROR "Central FW File format (${central_app_format_ver}) is not supported" )
        endif()

        # Read 4 bytes of header_size
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 0 4 central_app_header_size)

        # Read 4 bytes of data_size
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 4 4 central_app_data_size)

        # Read 4 bytes of version_size
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 12 4 central_app_version_size)
        if (NOT ${central_app_version_size} EQUAL 3 AND NOT ${central_app_version_size} EQUAL 7)
            message(FATAL_ERROR "Central FW version size (${central_app_version_size}) is not supported" )
        endif()

        # Read 1 byte of major,minor,patch versions
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 16 1 central_app_version_major)
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 17 1 central_app_version_minor)
        readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 18 1 central_app_version_patch)

        # Check if need to read read 4 bytes of build version
        if (${central_app_version_size} EQUAL 7)
            readBinHeaderField(${LIBTM_RESOURCES_DIR}/central_app.bin 19 4 central_app_version_build)
        endif()

        # Check if internal central FW version is compatible with file name
        if (NOT ${central_app_version_major} EQUAL ${central_app_major_version} OR 
            NOT ${central_app_version_minor} EQUAL ${central_app_minor_version} OR 
            NOT ${central_app_version_patch} EQUAL ${central_app_patch_version} OR 
            NOT ${central_app_version_build} EQUAL ${central_app_build_version})
            message(FATAL_ERROR "Wrong Central FW version (${central_app_version_major}.${central_app_version_minor}.${central_app_version_patch}.${central_app_version_build}) VS (${central_app_major_version}.${central_app_minor_version}.${central_app_patch_version}.${central_app_build_version})")
        endif()

        # message(STATUS "Central Header:")
        # message(STATUS "- header_size         = ${central_app_header_size}")
        # message(STATUS "- data_size           = ${central_app_data_size}")
        # message(STATUS "- file_format_version = ${central_app_format_ver}")
        # message(STATUS "- version_size        = ${central_app_version_size}")
        # message(STATUS "- version             = ${central_app_version_major}.${central_app_version_minor}.${central_app_version_patch}.${central_app_version_build}")

        # Create empty output file
        file(WRITE ${CENTRAL_APP_OUTPUT_FILE} "")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "/*******************************************************************************\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "INTEL CORPORATION PROPRIETARY INFORMATION\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "Copyright(c) 2017 Intel Corporation. All Rights Reserved.\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "*******************************************************************************/\n\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "#ifndef CentralAppFw_h\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "#define CentralAppFw_h\n\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "#define CENTRAL_APP_VERSION \"${CENTRAL_APP_VERSION}\"\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "#define CENTRAL_APP_SIZE ${central_app_data_size}\n\n")

        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "namespace CentralAppFw {\n\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "const int Version[] = { ${central_app_major_version}, ${central_app_minor_version}, ${central_app_patch_version}, ${central_app_build_version} };\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "const uint32_t Buffer []  = {\n")

        bin2h(${LIBTM_RESOURCES_DIR}/central_app.bin ${central_app_header_size} central_app_ready_buffer)
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "${central_app_ready_buffer}\n")

        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "};\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "}\n")
        file(APPEND ${CENTRAL_APP_OUTPUT_FILE} "#endif\n")
    else()
        if (NOT EXISTS "${CENTRAL_APP_OUTPUT_FILE}")
            message(FATAL_ERROR "File ${CENTRAL_APP_OUTPUT_FILE} is missing" )
        else()
            message(STATUS "Already created ${CENTRAL_APP_OUTPUT_FILE}")
        endif()
    endif()

endif(CENTRAL_APP_VERSION) 


if(CENTRAL_BL_VERSION)
    # message(STATUS "--------------------------------------------------------------------------------------------------------------------------------------------------------------")

    if (FW_SOURCE MATCHES "Remote")
        file(REMOVE ${LIBTM_RESOURCES_DIR}/central_bl.bin)
        message(STATUS "Downloading Central BL ${CENTRAL_BL_VERSION} from '${CENTRAL_BL_ARTIFACTORY_DIR}/${CENTRAL_BL_VERSION}/central_bl-${CENTRAL_BL_VERSION}.bin'")
        file(DOWNLOAD "${CENTRAL_BL_ARTIFACTORY_DIR}/${CENTRAL_BL_VERSION}/central_bl-${CENTRAL_BL_VERSION}.bin" "${LIBTM_RESOURCES_DIR}/central_bl.bin" TIMEOUT 60 STATUS status LOG log)
        # message(STATUS "Downloading Central BL ${CENTRAL_BL_VERSION} completed - status ${status}")
        list (FIND status "\"No error\"" _index)
        if (${_index} EQUAL -1)
           message(FATAL_ERROR "Download error ${CENTRAL_BL_ARTIFACTORY_DIR}/${CENTRAL_BL_VERSION}/central_bl-${CENTRAL_BL_VERSION}.bin" )
        endif()
    elseif (FW_SOURCE MATCHES "Local")
        if(NOT EXISTS "${LIBTM_RESOURCES_DIR}/central_bl.bin")
            message(FATAL_ERROR "File ${LIBTM_RESOURCES_DIR}/central_bl.bin is missing" )
        else()
            message(STATUS "Downloading Central BL ${CENTRAL_BL_VERSION} skipped")
        endif()
    endif()

    if (FW_SOURCE MATCHES "Remote" OR FW_SOURCE MATCHES "Local")
        file(REMOVE ${CENTRAL_BL_OUTPUT_FILE})
        message(STATUS "Converting Central BL version ${CENTRAL_BL_VERSION} from central_bl.bin to ${CENTRAL_BL_OUTPUT_FILE}")

        STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+\\.[0-9]+" "\\1" central_bl_major_version "${CENTRAL_BL_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" central_bl_minor_version "${CENTRAL_BL_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" central_bl_patch_version "${CENTRAL_BL_VERSION}")
        STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" central_bl_build_version "${CENTRAL_BL_VERSION}")

        # Create empty output file
        file(WRITE ${CENTRAL_BL_OUTPUT_FILE} "")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "/*******************************************************************************\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "INTEL CORPORATION PROPRIETARY INFORMATION\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "Copyright(c) 2017 Intel Corporation. All Rights Reserved.\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "*******************************************************************************/\n\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "#ifndef CentralBlFw_h\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "#define CentralBlFw_h\n\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "#define CENTRAL_BL_VERSION \"${CENTRAL_BL_VERSION}\"\n\n")
    
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "namespace CentralBlFw {\n\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "const int Version[] = { ${central_bl_major_version}, ${central_bl_minor_version}, ${central_bl_patch_version}, ${central_bl_build_version} };\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "const uint32_t Buffer []  = {\n")
    
        bin2h(${LIBTM_RESOURCES_DIR}/central_bl.bin 3 central_bl_ready_buffer)
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "${central_bl_ready_buffer}\n")

        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "};\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "}\n")
        file(APPEND ${CENTRAL_BL_OUTPUT_FILE} "#endif\n")
    else()
        if (NOT EXISTS "${CENTRAL_BL_OUTPUT_FILE}")
            message(FATAL_ERROR "File ${CENTRAL_BL_OUTPUT_FILE} is missing" )
        else()
            message(STATUS "Already created ${CENTRAL_BL_OUTPUT_FILE}")
        endif()
    endif()

endif(CENTRAL_BL_VERSION) 


# Update versions.log & remote_versions.log file
set(FWVERSIONS "FW [${FW_VERSION}]\nCentral App [${CENTRAL_APP_VERSION}]\nCentral BL [${CENTRAL_BL_VERSION}]\n")
set(ALLVERSIONS "Host [${HOST_VERSION}]\nFW [${FW_VERSION}]\nCentral App [${CENTRAL_APP_VERSION}]\nCentral BL [${CENTRAL_BL_VERSION}]\n")

if (FW_SOURCE MATCHES "Remote")
    file(WRITE ${LIBTM_RESOURCES_DIR}/remote_versions.log ${FWVERSIONS})
    file(WRITE ${LIBTM_ROOT}/versions.log ${ALLVERSIONS})
elseif (FW_SOURCE MATCHES "Local")
    file(WRITE ${LIBTM_ROOT}/versions.log Host ${ALLVERSIONS})
elseif (FW_SOURCE MATCHES "Internal")
    file(WRITE ${LIBTM_ROOT}/versions.log ${ALLVERSIONS})
else()
    message(FATAL_ERROR "Bad FW source - ${FW_SOURCE}" )
endif()
