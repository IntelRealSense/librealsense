# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

import sys
import time
import pyrealsense2 as rs
from rspy import test, log
from test_calibrations_common import calibration_main, is_mipi_device


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
        log.i('Using default parameters for on-chip calibration.')
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
    # TODO - host assistance actual value may be different when reading from json
    return occ_json


# Health factor threshold for calibration success
HEALTH_FACTOR_THRESHOLD = 0.25

with test.closure("OCC calibration test"):
    try:        
        host_assistance = False

        if is_mipi_device():
            log.i("MIPI device - skip the test w/o host assistance")
            test.skip()

        occ_json = on_chip_calibration_json(None, host_assistance)
        health_factor = calibration_main(host_assistance, True, occ_json, None)
        test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
    except Exception as e:
        log.e("OCC calibration test failed: ", str(e))
        test.fail()

with test.closure("OCC calibration test with host assistance"):
    try:
        host_assistance = True
        occ_json = on_chip_calibration_json(None, host_assistance)
        health_factor = calibration_main(host_assistance, True, occ_json, None)
        test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)     
    except Exception as e:
        log.e("OCC calibration test with host assistance failed: ", str(e))
        test.fail()

"""
with test.closure("OCC calibration with table backup and modification"):
    try:
        host_assistance = False
        occ_json = on_chip_calibration_json(None, host_assistance)
        
        log.i("Starting OCC calibration with calibration table backup/restore demonstration")
        health_factor = perform_calibration_with_table_backup(host_assistance, True, occ_json, None)
        
        if health_factor is not None:
            test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
            log.i("OCC calibration with table manipulation completed successfully")
        else:
            log.e("Calibration with table backup failed")
            test.fail()
            
    except Exception as e:
        log.e("OCC calibration with table backup failed: ", str(e))
        test.fail()
    log.i("Done\n")

"""
test.print_results_and_exit()


