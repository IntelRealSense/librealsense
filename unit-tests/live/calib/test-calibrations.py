## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#########################################################################################################################################
##                   tests for OCC and Tare calib flows                                                                ##
#########################################################################################################################################
import sys
import time
import pyrealsense2 as rs
from rspy import test, log

#disabled until we stabilize lab
#test:donotrun

def on_chip_calibration_json(occ_json_file, host_assistance, interactive_mode):
    occ_json = None
    if occ_json_file is not None:
        try:
            occ_json = open(occ_json_file).read()
        except:
            occ_json = None
            if occ_json_file:
                print('Error reading occ_json_file: ', occ_json_file)
        
    if occ_json is None:
        print ('Using default parameters for on-chip calibration.')
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
                    '"interactive scan": ' + str(int(interactive_mode)) + ',\n'+\
                    '"resize factor": 1\n'+\
                    '}'
    return occ_json

def tare_calibration_json(tare_json_file, host_assistance):
    tare_json = None
    if tare_json_file is not None:
        try:
            tare_json = open(tare_json_file).read()
        except:
            tare_json = None
            if tare_json_file:
                print('Error reading tare_json_file: ', tare_json_file)
    if tare_json is None:
        print ('Using default parameters for Tare calibration.')
        tare_json = '{\n  '+\
                    '"host assistance": ' + str(int(host_assistance)) + ',\n'+\
                    '"speed": 3,\n'+\
                    '"scan parameter": 0,\n'+\
                    '"step count": 20,\n'+\
                    '"apply preset": 1,\n'+\
                    '"accuracy": 2,\n'+\
                    '"depth": 0,\n'+\
                    '"resize factor": 1\n'+\
                    '}'
    return tare_json

def on_chip_calib_cb(progress):
    pp = int(progress)
    sys.stdout.write('\r' + '*'*pp + ' '*(99-pp) + '*')
    if (pp == 100):
        print()

def calibration_main(host_assistance, occ, ground_truth):
    config = rs.config()
    pipeline = rs.pipeline()
    pipeline_wrapper = rs.pipeline_wrapper(pipeline)
    pipeline_profile = config.resolve(pipeline_wrapper)
    auto_calibrated_device = rs.auto_calibrated_device(pipeline_profile.get_device())
    if not auto_calibrated_device:
        test.fail("Failed to open device for calibration")
        test.finish()
    ctx = rs.context()
    try:
        device = ctx.query_devices()[0]
    except IndexError:
        test.fail("Failed to connect device for calibration")
        test.finish()
    # Bring device in start phase
    device.hardware_reset()
    time.sleep(3)

    #params = dict(zip(sys.argv[1::2], sys.argv[2::2]))
    occ_json_file = None
    tare_json_file = None

    interactive_mode = False

    stream_config = rs.config()
    stream_config.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)

    conf = pipeline.start(stream_config)
    pipeline.wait_for_frames() # Verify streaming started before calling calibration methods
    calib_dev = rs.auto_calibrated_device(conf.get_device())

    # prepare device
    thermal_compensation    = 0
    emitter                 = 0

    depth_sensor = conf.get_device().first_depth_sensor()
    if depth_sensor.supports(rs.option.emitter_enabled):
        emitter = depth_sensor.get_option(rs.option.emitter_enabled)
        depth_sensor.set_option(rs.option.emitter_enabled, 1)
    if depth_sensor.supports(rs.option.thermal_compensation):
        thermal_compensation = depth_sensor.get_option(rs.option.thermal_compensation)
        depth_sensor.set_option(rs.option.thermal_compensation, 0)


    if occ:
        print("Starting on chip calibration")
        occ_json = on_chip_calibration_json(occ_json_file, host_assistance, interactive_mode)
        timeout = 9000
        new_calib, health = calib_dev.run_on_chip_calibration(occ_json, on_chip_calib_cb, timeout)
    else:
        print("Starting tare calibration")
        tare_json = tare_calibration_json(tare_json_file, host_assistance)
        new_calib, health = calib_dev.run_tare_calibration(ground_truth, tare_json, on_chip_calib_cb, 10000)

    calib_done = len(new_calib) > 0
    timeout = time.time() + 30  # 30 seconds timeout
    while not calib_done:
        if time.time() > timeout:
            raise RuntimeError("Calibration timed out")
        frame_set = pipeline.wait_for_frames()
        depth_frame = frame_set.get_depth_frame()
        new_calib, health = calib_dev.process_calibration_frame(depth_frame, on_chip_calib_cb, 5000)
        calib_done = len(new_calib) > 0
        
    print("Calibration completed")
    print("health factor = ", health[0])
                 
    # revert device in previous state
    depth_sensor = conf.get_device().first_depth_sensor()
    if depth_sensor.supports(rs.option.emitter_enabled):
        depth_sensor.set_option(rs.option.emitter_enabled, emitter)
    if depth_sensor.supports(rs.option.thermal_compensation):
        depth_sensor.set_option(rs.option.thermal_compensation, thermal_compensation)

    pipeline.stop()

    return health[0]


def calculate_target_z():
    number_of_images = 50 # The required number of frames is 10+
    timeout_s = 30
    target_size = [175, 100]

    cfg = rs.config()
    cfg.enable_stream(rs.stream.infrared, 1, 1280, 720, rs.format.y8, 30)

    q = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    q2 = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    q3 = rs.frame_queue(capacity=number_of_images, keep_frames=True)

    counter = 0

    def cb(frame):
        nonlocal counter
        if counter > number_of_images:
            return
        for f in frame.as_frameset():
            q.enqueue(f)
            counter += 1

    ctx = rs.context()
    pipe = rs.pipeline(ctx)
    pp = pipe.start(cfg, cb)
    dev = pp.get_device()

    try:
        stime = time.time()
        while counter < number_of_images:
            time.sleep(0.5)
            if timeout_s < (time.time() - stime):
                raise RuntimeError(f"Failed to capture {number_of_images} frames in {timeout_s} seconds, got only {counter} frames")

        adev = dev.as_auto_calibrated_device()
        print('Calculating distance to target...')
        print(f'\tTarget Size:\t{target_size}')
        target_z = adev.calculate_target_z(q, q2, q3, target_size[0], target_size[1])
        print(f'Calculated distance to target is {target_z}')
    finally:
        pipe.stop()

    return target_z

with test.closure("OCC calibration test"):
    host_assistance = False
    health_factor = calibration_main(host_assistance, True, None)
    test.check(abs(health_factor) < 0.5)        
    print("Done\n")

with test.closure("OCC calibration test with host assistance"):
    host_assistance = True
    health_factor = calibration_main(host_assistance, True, None)
    test.check(abs(health_factor) < 0.5)     
    print("Done\n")


with test.closure("Tare calibration test with host assistance"):
    host_assistance = True

    target_z = calculate_target_z()
    test.check(target_z > 0 and target_z < 1500)   
    health_factor = calibration_main(host_assistance, False, target_z)
    test.check(abs(health_factor) < 0.5)     
    print("Done\n")

with test.closure("Tare calibration test"):
    host_assistance = False
    target_z = calculate_target_z()
    test.check(target_z > 0 and target_z < 1500)     
    health_factor = calibration_main(host_assistance, False, target_z)
    test.check(abs(health_factor) < 0.5)        
    print("Done\n")

test.print_results_and_exit()


