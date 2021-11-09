## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#####################################################
##                   auto calibration              ##
#####################################################

import argparse
import json
import sys
import time

import pyrealsense2 as rs

__desc__ = """
This script demonstrates usage of Self-Calibration (UCAL) APIs
"""

# mappings
occ_speed_map = {
    'very_fast': 0,
    'fast': 1,
    'medium': 2,
    'slow': 3,
    'wall': 4,
}
tare_accuracy_map = {
    'very_high': 0,
    'high': 1,
    'medium': 2,
    'low': 3,
}
scan_map = {
    'intrinsic': 0,
    'extrinsic': 1,
}
fl_adjust_map = {
    'right_only': 0,
    'both_sides': 1
}

ctx = rs.context()


def main(arguments=None):
    args = parse_arguments(arguments)

    try:
        device = ctx.query_devices()[0]
    except IndexError:
        print('Device is not connected')
        sys.exit(1)

    # Verify Preconditions:
    # 1. The script is applicable for D400-series devices only
    cam_name = device.get_info(rs.camera_info.name) if device.supports(rs.camera_info.name) else "Unrecognized camera"
    if device.supports(rs.camera_info.product_line):
        device_product_line = str(device.get_info(rs.camera_info.product_line))
        if device_product_line != 'D400':
            print(f'The example is intended for RealSense D400 Depth cameras, and is not', end =" ")
            print(f'applicable with {cam_name}')
            sys.exit(1)
    # 2. The routine assumes USB3 connection type
    #    In case of USB2 connection, the streaming profiles should be readjusted
    if device.supports(rs.camera_info.usb_type_descriptor):
        usb_type = device.get_info(rs.camera_info.usb_type_descriptor)
        if not usb_type.startswith('3.'):
            print('The script is designed to run with USB3 connection type.')
            print('In order to enable it with USB2.1 mode the fps rates for the Focal Length and Ground Truth calculation stages should be re-adjusted')
            sys.exit(1)


    # prepare device
    depth_sensor = device.first_depth_sensor()
    depth_sensor.set_option(rs.option.emitter_enabled, 0)
    if depth_sensor.supports(rs.option.thermal_compensation):
        depth_sensor.set_option(rs.option.thermal_compensation, 0)
    if args.exposure == 'auto':
        depth_sensor.set_option(rs.option.enable_auto_exposure, 1)
    else:
        depth_sensor.set_option(rs.option.enable_auto_exposure, 0)
        depth_sensor.set_option(rs.option.exposure, int(args.exposure))

    print("Starting UCAL...")
    try:
        # The recomended sequence of procedures: On-Chip -> Focal Length -> Tare Calibration
        run_on_chip_calibration(args.onchip_speed, args.onchip_scan)
        run_focal_length_calibration((args.target_width, args.target_height), args.focal_adjustment)
        run_tare_calibration(args.tare_accuracy, args.tare_scan, args.tare_gt, (args.target_width, args.target_height))
    finally:
        if depth_sensor.supports(rs.option.thermal_compensation):
            depth_sensor.set_option(rs.option.thermal_compensation, 1)
    print("UCAL finished successfully")


def progress_callback(progress):
    print(f'\rProgress  {progress}% ... ', end ="\r")

def run_on_chip_calibration(speed, scan):
    data = {
        'calib type': 0,
        'speed': occ_speed_map[speed],
        'scan parameter': scan_map[scan],
        'white_wall_mode': 1 if speed == 'wall' else 0,
    }

    args = json.dumps(data)

    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)
    pipe = rs.pipeline(ctx)
    pp = pipe.start(cfg)
    dev = pp.get_device()

    try:

        print('Starting On-Chip calibration...')
        print(f'\tSpeed:\t{speed}')
        print(f'\tScan:\t{scan}')
        adev = dev.as_auto_calibrated_device()
        table, health = adev.run_on_chip_calibration(args, progress_callback, 30000)
        print('On-Chip calibration finished')
        print(f'\tHealth: {health}')
        adev.set_calibration_table(table)
        adev.write_calibration()
    finally:
        pipe.stop()


def run_focal_length_calibration(target_size, adjust_side):
    number_of_images = 25
    timeout_s = 30

    cfg = rs.config()
    cfg.enable_stream(rs.stream.infrared, 1, 1280, 720, rs.format.y8, 30)
    cfg.enable_stream(rs.stream.infrared, 2, 1280, 720, rs.format.y8, 30)

    lq = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    rq = rs.frame_queue(capacity=number_of_images, keep_frames=True)

    counter = 0
    flags = [False, False]

    def cb(frame):
        nonlocal counter, flags
        if counter > number_of_images:
            return
        for f in frame.as_frameset():
            p = f.get_profile()
            if p.stream_index() == 1:
                lq.enqueue(f)
                flags[0] = True
            if p.stream_index() == 2:
                rq.enqueue(f)
                flags[1] = True
            if all(flags):
                counter += 1
        flags = [False, False]

    pipe = rs.pipeline(ctx)
    pp = pipe.start(cfg, cb)
    dev = pp.get_device()

    try:
        print('Starting Focal-Length calibration...')
        print(f'\tTarget Size:\t{target_size}')
        print(f'\tSide Adjustment:\t{adjust_side}')
        stime = time.time()
        while counter < number_of_images:
            time.sleep(0.5)
            if timeout_s < (time.time() - stime):
                raise RuntimeError(f"Failed to capture {number_of_images} frames in {timeout_s} seconds, got only {counter} frames")

        adev = dev.as_auto_calibrated_device()
        table, ratio, angle = adev.run_focal_length_calibration(lq, rq, target_size[0], target_size[1],
                                                                fl_adjust_map[adjust_side],progress_callback)
        print('Focal-Length calibration finished')
        print(f'\tRatio:\t{ratio}')
        print(f'\tAngle:\t{angle}')
        adev.set_calibration_table(table)
        adev.write_calibration()
    finally:
        pipe.stop()


def run_tare_calibration(accuracy, scan, gt, target_size):
    data = {'scan parameter': scan_map[scan],
            'accuracy': tare_accuracy_map[accuracy],
            }
    args = json.dumps(data)

    print('Starting Tare calibration...')
    if gt == 'auto':
        target_z = calculate_target_z(target_size)
    else:
        target_z = float(gt)

    cfg = rs.config()
    cfg.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)
    pipe = rs.pipeline(ctx)
    pp = pipe.start(cfg)
    dev = pp.get_device()

    try:
        print(f'\tGround Truth:\t{target_z}')
        print(f'\tAccuracy:\t{accuracy}')
        print(f'\tScan:\t{scan}')
        adev = dev.as_auto_calibrated_device()
        table = adev.run_tare_calibration(target_z, args, progress_callback, 30000)
        print('Tare calibration finished')
        adev.set_calibration_table(table)
        adev.write_calibration()

    finally:
        pipe.stop()


def calculate_target_z(target_size):
    number_of_images = 50 # The required number of frames is 10+
    timeout_s = 30

    cfg = rs.config()
    cfg.enable_stream(rs.stream.infrared, 1, 1280, 720, rs.format.y8, 30)

    q = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    # Frame queues q2, q3 should be left empty. Provision for future enhancements.
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


def parse_arguments(args):
    parser = argparse.ArgumentParser(description=__desc__)

    parser.add_argument('--exposure', default='auto', help="Exposure value or 'auto' to use auto exposure")
    parser.add_argument('--target-width', default=175, type=int, help='The target width')
    parser.add_argument('--target-height', default=100, type=int, help='The target height')

    parser.add_argument('--onchip-speed', default='medium', choices=occ_speed_map.keys(), help='On-Chip speed')
    parser.add_argument('--onchip-scan', choices=scan_map.keys(), default='intrinsic', help='On-Chip scan')

    parser.add_argument('--focal-adjustment', choices=fl_adjust_map.keys(), default='right_only',
                        help='Focal-Length adjustment')

    parser.add_argument('--tare-gt', default='auto',
                        help="Target ground truth, set 'auto' to calculate using target size"
                             "or the distance to target in mm to use a custom value")
    parser.add_argument('--tare-accuracy', choices=tare_accuracy_map.keys(), default='medium', help='Tare accuracy')
    parser.add_argument('--tare-scan', choices=scan_map.keys(), default='intrinsic', help='Tare scan')

    return parser.parse_args(args)


if __name__ == '__main__':
    main()
