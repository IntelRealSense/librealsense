# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#test:device each(D400*)
#test:device each(D500*)

import pyrealsense2 as rs
from rspy import log, repo, test
from rspy.stopwatch import Stopwatch

#############################################################################################
#
test.start( "Run enumerate-devices runtime test" )
rs_enumerate_devices = repo.find_built_exe( 'tools/enumerate-devices', 'rs-enumerate-devices' )
test.check(rs_enumerate_devices)
if rs_enumerate_devices:
    dev, ctx = test.find_first_device_or_exit()
    is_dds = dev.supports(rs.camera_info.connection_type) and dev.get_info(rs.camera_info.connection_type) == "DDS"
    import subprocess
    run_time_stopwatch = Stopwatch()
    run_time_threshold = 5 if is_dds else 2  # currently, DDS devices take longer time to complete rs_enumerate_devices
    p = subprocess.run( [rs_enumerate_devices, "--no-dds" if not is_dds else ""],
                    stdout=None,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                    timeout=10,
                    check=False )  # don't fail on errors
    test.check(p.returncode == 0) # verify success return code
    run_time_seconds = run_time_stopwatch.get_elapsed()
    if run_time_seconds > run_time_threshold:
        log.e('Time elapsed too high!', run_time_seconds, ' > ', run_time_threshold)
    log.d("rs-enumerate-devices completed in:", run_time_seconds, "seconds")
    test.check(run_time_seconds < run_time_threshold)
    run_time_stopwatch.reset()
else:
    log.e( 'no rs-enumerate-devices was found!' )
    import sys
    log.d( 'sys.path=\n    ' + '\n    '.join( sys.path ) )

test.finish()
#
#############################################################################################
test.print_results_and_exit()
