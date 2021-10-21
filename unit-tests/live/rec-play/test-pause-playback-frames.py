# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#test:device L500*
#test:device D400*

import pyrealsense2 as rs, os, time, tempfile, platform, sys
from rspy import devices, log, test



def stop_pipeline( pipeline ):
    if pipeline:
        try:
            pipeline.stop()
        except RuntimeError as rte:
            # if the error Occurred because the pipeline wasn't started we ignore it
            if str( rte ) != "stop() cannot be called before start()":
                test.unexpected_exception()
        except Exception:
            test.unexpected_exception()

file_name = 'c:/test/rec.bag'
################################################################################################
test.start("Trying to record pause and resume test using pipeline interface")

cfg = pipeline = None
try:
    # creating a pipeline and recording to a file
    pipeline = rs.pipeline()
    cfg = rs.config()
    cfg.enable_record_to_file( file_name )
    pipeline_record_profile = pipeline.start( cfg )
    device_record = pipeline_record_profile.get_device()
    device_recorder = device_record.as_recorder()
    print('Pausing.', end='')
    rs.recorder.pause(device_recorder)
    time.sleep(2)
    print('Resumed.', end='')
    rs.recorder.resume(device_recorder)
    time.sleep(5)
    pipeline.stop()
except Exception:
    test.unexpected_exception()
finally: # we must remove all references to the file so we can use it again in the next test
    cfg = None
    stop_pipeline( pipeline )

test.finish()

#############################################################################################
test.print_results_and_exit()
