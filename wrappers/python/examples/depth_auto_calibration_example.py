## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#####################################################
##                   auto calibration              ##
#####################################################

# First import the library
import pyrealsense2 as rs
import  sys, getopt
def read_parameters(argv):
    opts, args = getopt.getopt(argv, "i:", ["ifile="])
    jcnt = ''
    size = 0

    if len(opts) > 0:
        json_file = opts[0][1]
        file_object = open(json_file)
        jcnt = file_object.read()
        size = len(jcnt)

    return size, jcnt

def main(argv):

    size, file_cnt = read_parameters(argv)

    pipeline = rs.pipeline()
    config = rs.config()

    config.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)
    conf = pipeline.start(config)
    calib_dev = rs.auto_calibrated_device(conf.get_device())

    def on_chip_calib_cb(progress):
        print(". ")

    while True:
        try:
            input = raw_input("Please select what the operation you want to do\nc - on chip calibration\nt - tare calibration\ng - get the active calibration\nw - write new calibration\ne - exit\n")

            if input == 'c':
                print("Starting on chip calibration")
                new_calib, health = calib_dev.run_on_chip_calibration(5000, file_cnt, on_chip_calib_cb)
                print("Calibration completed")
                print("health factor = ", health)

            if input == 't':
                print("Starting tare calibration")
                ground_truth = float(raw_input("Please enter ground truth in mm\n"))
                new_calib, health = calib_dev.run_tare_calibration(ground_truth, 5000, file_cnt, on_chip_calib_cb)
                print("Calibration completed")
                print("health factor = ", health)

            if input == 'g':
                calib = calib_dev.get_calibration_table()
                print("Calibration", calib)

            if input == 'w':
                print("Writing the new calibration")
                calib_dev.set_calibration_table(new_calib)
                calib_dev.write_calibration()

            if input == 'e':
                pipeline.stop()
                return

            print("Done\n")
        except Exception as e:
            pipeline.stop()
            print(e)
        except:
            pipeline.stop()
            print("A different Error")




if __name__ == "__main__":
    main(sys.argv[1:])
