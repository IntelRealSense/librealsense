# Copyright (2017-2025), RealSense, Inc.
# Certain Intel® RealSense™ products are sold by RealSense, Inc. under license from Intel Corporation),
# This "Software" is furnished under license and may only be used or copied in accordance with the terms of that license.
# No license, express or implied, by estoppel or otherwise, to any intellectual property rights is granted by this document.
# The Software is subject to change without notice and should not be construed as a commitment by RealSense, Inc. or Intel Corporation to market, license, sell or support any product or technology.
# Unless otherwise provided for in the license under which this Software is provided, the Software is provided AS IS, with no warranties of any kind, express or implied.
# Except as expressly permitted by the Software license, neither RealSense, Inc. nor Intel Corporation, or any of their suppliers, assumes any responsibility or liability for any errors or inaccuracies that may appear herein.
# Except as expressly permitted by the Software license, no part of the Software may be reproduced, stored in a retrieval system, transmitted in any form, or distributed by any means without the express written consent of RealSense, Inc.

#########################################################################################################################################
##                   tests for OCC and Tare calib flows                                                                ##
#########################################################################################################################################
import sys
import time
import pyrealsense2 as rs
from rspy import test, log
from test_calibrations_common import calibration_main

#disabled until we stabilize lab
#test:donotrun

def on_chip_calibration_json(occ_json_file, host_assistance):
    occ_json = None
    if occ_json_file is not None:
        try:
            occ_json = open(occ_json_file).read()
        except:
            occ_json = None
            log.e('Error reading occ_json_file: ', occ_json_file)
        
    if occ_json is None:
        log.i ('Using default parameters for on-chip calibration.')
        occ_json = '{\n  '+\
                    '"calib type": 0,\n'+\
                    '"host assistance": ' + str(int(host_assistance)) + ',\n'+\
                    '"keep new value after successful scan": 0,\n'+\
                    '"fl data sampling": 0,\n'+\
                    '"adjust both sides": 0,\n'+\
                    '"fl scan location": 0,\n'+\
                    '"fy scan direction": 0,\n'+\
                    '"white wall mode": 0,\n'+\
                    '"speed": 2,\n'+\
                    '"scan parameter": 0,\n'+\
                    '"apply preset": 0,\n'+\
                    '"scan only": ' + str(int(host_assistance)) + ',\n'+\
                    '"interactive scan": 0,\n'+\
                    '"resize factor": 1\n'+\
                    '}'
    return occ_json


# Health factor threshold for calibration success
HEALTH_FACTOR_THRESHOLD = 0.25

with test.closure("OCC calibration test"):
    try:
        host_assistance = False
        occ_json = on_chip_calibration_json(None, host_assistance)
        status, health_factor = calibration_main(host_assistance, True, occ_json, None)
        if (status):
            test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
            log.i("OCC calibration test completed")
    except Exception as e:
        log.e("OCC calibration test failed: ", str(e))
        test.fail()
    log.i("Done\n")

with test.closure("OCC calibration test with host assistance"):
    try:
        host_assistance = True
        occ_json = on_chip_calibration_json(None, host_assistance)
        status, health_factor = calibration_main(host_assistance, True, occ_json, None)
        if (status):
            test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)     
            log.i("OCC calibration test with host assistance completed")
        else:
            log.e("Unexpected skip")
            test.fail()
    except Exception as e:
        log.e("OCC calibration test with host assistance failed: ", str(e))
        test.fail()
    log.i("Done\n")


test.print_results_and_exit()


