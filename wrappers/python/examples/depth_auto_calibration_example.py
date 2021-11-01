## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#####################################################
##                   auto calibration              ##
#####################################################

# First import the library
import pyrealsense2 as rs
import  sys

def tare_calibration_json(tare_json_file, host_assistance):
    try:
        tare_json = open(tare_json_file).read()
    except:
        if tare_json_file:
            print('Error reading tare_json_file: ', tare_json_file)
        print ('Using default parameters for Tare calibration.')
        tare_json = '{\n  '+\
                    '"calib type": 2,\n'+\
                    '"host assistance": ' + str(int(host_assistance)) + ',\n'+\
                    '"keep new value after sucessful scan": 1,\n'+\
                    '"fl data sampling": 1,\n'+\
                    '"adjust both sides": 0,\n'+\
                    '"fl scan location": 0,\n'+\
                    '"fy scan direction": 0,\n'+\
                    '"white wall mode": 0,\n'+\
                    '"speed": 3,\n'+\
                    '"average_step_count": 20,\n'+\
                    '"scan parameter": 0,\n'+\
                    '"step count": 20,\n'+\
                    '"apply preset": 1,\n'+\
                    '"accuracy": 2,\n'+\
                    '"scan only": ' + str(int(host_assistance)) + ',\n'+\
                    '"interactive scan": 0,\n'+\
                    '"depth": 0\n'+\
                    '}'
    return tare_json

def main(argv):
    if '--help' in sys.argv or '-h' in sys.argv:
        print('USAGE:')
        print('depth_auto_calibration_example.py --occ <json_file_name> --tare <json_file_name>')
        print
        sys.exit(-1)

    params = dict(zip(sys.argv[1::2], sys.argv[2::2]))
    occ_json_file = params.get('--occ', None)
    tare_json_file = params.get('--tare', None)

    if occ_json_file:
        occ_json = open(occ_json_file).read()

    pipeline = rs.pipeline()
    config = rs.config()

    # Get device product line for setting a supporting resolution
    pipeline_wrapper = rs.pipeline_wrapper(pipeline)
    pipeline_profile = config.resolve(pipeline_wrapper)
    device = pipeline_profile.get_device()

    auto_calibrated_device = rs.auto_calibrated_device(device)

    if not auto_calibrated_device:
        print("The connected device does not support auto calibration")
        return

    config.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)
    conf = pipeline.start(config)
    calib_dev = rs.auto_calibrated_device(conf.get_device())

    def on_chip_calib_cb(progress):
        print(". ")

    while True:
        try:
            operation_str = "Please select what the operation you want to do\n" + \
                            "c - on chip calibration\n" + \
                            "t - tare calibration\n" + \
                            "T - tare calibration - host assist\n" + \
                            "g - get the active calibration\n" + \
                            "w - write new calibration\n" + \
                            "e - exit\n"
            operation = input(operation_str)

            if operation == 'c':
                print("Starting on chip calibration")
                new_calib, health = calib_dev.run_on_chip_calibration(occ_json, on_chip_calib_cb, 5000)
                calib_done = len(new_calib) > 0
                while (not calib_done):
                    frame_set = pipeline.wait_for_frames()
                    depth_frame = frame_set.get_depth_frame()
                    new_calib, health = calib_dev.add_calibration_frame(depth_frame, 5000)
                    calib_done = len(new_calib) > 0
                print("Calibration completed")
                print("health factor = ", health)

            if operation.lower() == 't':
                print("Starting tare calibration" + (" - host assistance" if operation == 'T' else ""))
                ground_truth = float(input("Please enter ground truth in mm\n"))
                tare_json = tare_calibration_json(tare_json_file, operation == 'T')
                new_calib, health = calib_dev.run_tare_calibration(ground_truth, tare_json, on_chip_calib_cb, 5000)
                calib_done = len(new_calib) > 0
                while (not calib_done):
                    frame_set = pipeline.wait_for_frames()
                    depth_frame = frame_set.get_depth_frame()
                    new_calib, health = calib_dev.add_calibration_frame(depth_frame, 5000)
                    calib_done = len(new_calib) > 0
                print("Calibration completed")
                print("health factor = ", health)

            if operation == 'g':
                calib = calib_dev.get_calibration_table()
                print("Calibration", calib)

            if operation == 'w':
                print("Writing the new calibration")
                calib_dev.set_calibration_table(new_calib)
                calib_dev.write_calibration()

            if operation == 'e':
                pipeline.stop()
                return

            print("Done\n")
        except Exception as e:
            print(e)
        except:
            print("A different Error")




if __name__ == "__main__":
    main(sys.argv[1:])
