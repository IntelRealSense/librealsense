import os, sys
import subprocess
import re
import pyrealsense2 as rs
import pyrsutils as rsutils
from rspy import test, log, repo

with test.closure("Test to read SMCU Version from rsutils FW version, HWMC Commands and rs-fw-update tool, "
                    "Compare against 0.0.0.0 and check if all the methods provide same result"):
    versions_dict ={}

    # Find the first RealSense device or exit if not found
    device = test.find_first_device_or_exit()

    ########################  FW version using rs camera info  #######################

    # Get the firmware version using rs_camera_info
    fw_version = rsutils.version(device.get_info(rs.camera_info.smcu_fw_version))

    # Print the firmware version
    log.d("\n Firmware Version Using rs_camera_info: \n", fw_version)
    versions_dict["smcu_version_rsutils_fw_version"] = str(fw_version)

    ########################  FW version using rs camera info  #######################


    ############################  FW version using HMC  ################################

    # Get the firmware version using GVD cmd
    dp_device = device.as_debug_protocol()
    Data = dp_device.send_and_receive_raw_data(dp_device.build_command(0x10))
    #log.d ("HMC Command GVD " + str(Data))
    substring = Data[275:279]
    SMCU_Version_HMC = ".".join(str(char) for char in substring)
    log.d("\n Smcu Version Using HMC: \n", SMCU_Version_HMC)
    versions_dict["smcu_version_HMC"] = str(SMCU_Version_HMC)

    ############################  FW version using HMC  ################################


    ########################## FW version using rs-fw-update  ###########################

    # Get the firmware version using rs-fw-update
    if sys.platform.startswith('linux'):
        fw_update_tool = "rs-fw-update"
    elif sys.platform.startswith('win32'):
        fw_update_tool = "rs-fw-update.exe"
    else :
        log.d("\n Other platforms are not supported \n")
        test.abort()
    rs_fw_update_tool_path = os.path.join(os.getcwd(), "..", "build", "Release", fw_update_tool)
    #log.d ("rs-fw-Update path in the directory :",rs_fw_update_tool_path)
    Updated_version = subprocess.check_output(rs_fw_update_tool_path)
    log.d("\n fw-update info : \n", Updated_version)

    version = subprocess.check_output(rs_fw_update_tool_path).decode("utf-8")

    Output_version = re.search('SMCU firmware version: ([\d.]+)',version)
    if Output_version:
        Smcu_version= Output_version.group(1)
        log.d("\n  SMCU firmware version using rs-fw-update: \n", Smcu_version)
        versions_dict["smcu_version_rs_fw_update"] = str(Smcu_version)

    ########################## FW version using rs-fw-update  ###########################


    # check the output is not zero for all instances
    result = True
    for key, value in versions_dict.items():
        if value != "0.0.0.0":
            log.d("\nValid version :",key)
        else:
            result = False
            log.d("\nInvalid version :",key)
            break
    test.check_equal(result, True)

    # Check all the versions are equal
    log.d ("\n Actual Dictonary is " + str(versions_dict.values()))

    test_val = list(versions_dict.values())[0]
    log.d ("SMCU Version From one of the method = " + str(test_val))

    result = True
    for element in versions_dict:
        log.d ("SMCU Version = " + str(versions_dict[element]) + " for the key = " + str(element))
        if versions_dict[element] != test_val:
            log.d ("Version Mismatch for the key =" + str(element))
            result = False
            break
    test.check_equal(result, True)
    log.d ("Are all values similar = " + str(result))

test.print_results_and_exit()
