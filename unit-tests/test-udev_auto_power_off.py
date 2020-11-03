import os
import sys
import pyrealsense2 as rs
import time
from functools import partial

class udevTester:
    def __init__(self):
        self.ctx = rs.context()
        self.ctx.set_devices_changed_callback(self.devices_changed_callback)
        self.changed = False
        self.frame_count = dict()
        self.frame = None

    def devices_changed_callback(self, event):
        print ('Changed.')
        self.changed = True
        return None

    def message(self):
        print_help = '--help' in sys.argv or '-h' in sys.argv
        if print_help:
            print ('USAGE:')
            print ('python test_udev_auto_power_off.py [--two_devices]')
            print
            print ('Test application for udev-rules auto-power-off:')
            print ('Test that once a device is connected, all its IMU sensors are turned off.')
            print ('If --two_devices is set, test also that if a device is already connected and running,\
                    it is not turned off by connecting a new device.')
            print
            exit(-1)

    # def disconnect_all_devices(self):
    #     devices = self.ctx.query_devices()
    #     while (devices):
    #         print ('Please disconnect all realsense devices...')
    #         while (not self.changed):
    #             time.sleep(0.001)
    #         self.changed = False
    #         devices = self.ctx.query_devices()
            
    def is_motion_module_device(self, device):
        try:
            sensor = device.first_motion_sensor()
            return True
        except:
            return False

    def connect_imu_device(self, existing_sns):
        #function retrievs 1 motion module device that is not on the <existing_sns> list.
        imu_devices = [dev for dev in self.ctx.query_devices() if self.is_motion_module_device(dev) and dev.get_info(rs.camera_info.serial_number) not in existing_sns]
        start_waiting_time =time.time()
        time_out = 10
        while (len(imu_devices) == 0 and time.time() - start_waiting_time < time_out):
            print ('Please Connect 1 device with a motion module%s') % ('.' if len(existing_sns) == 0 else ' (other then: %s)' % (', '.join(existing_sns)))
            print ('Will wait for %.1f seconds...' % time_out)
            while (not self.changed and (time.time() - start_waiting_time < time_out)):
                time.sleep(0.001)
            self.changed = False
            imu_devices = [dev for dev in self.ctx.query_devices() if self.is_motion_module_device(dev) and dev.get_info(rs.camera_info.serial_number) not in existing_sns]
        if (len(imu_devices) == 0):
            return None
        return imu_devices[-1]

    def wait_for_device(self, sn):
        while (not self.changed):
            time.sleep(0.001)
        self.changed = False
        device = [dev for dev in self.ctx.query_devices() if dev.get_info(rs.camera_info.serial_number) == sn]
        if (len(device) == 0):
            sys.stdout.write('Waiting.')
        while (len(device) == 0):
            sys.stdout.write('.')
            time.sleep(0.001)
            device = [dev for dev in self.ctx.query_devices() if dev.get_info(rs.camera_info.serial_number) == sn]
        sys.stdout.write('\n')
        return device[0]

    def reset_device(self, dev):
        sn = dev.get_info(rs.camera_info.serial_number)
        print ('Reset Device: %s' % sn)
        dev.hardware_reset()
        # wait for device to disappear:
        dev = self.wait_for_device(sn)
        # # wait for device to reappear:
        # while (not self.changed):
        #     time.sleep(0.001)
        # self.changed = False
        sn = dev.get_info(rs.camera_info.serial_number)
        print ('Device %s finished reset.' % sn)
        return dev

    def check_power_off(self, ignore_dev1=False):
        enable_files = [os.path.join(dirpath,filename) for dirpath, _, filenames in os.walk('/sys') for filename in filenames if 'HID-SENSOR' in dirpath and filename == 'enable']
        if ignore_dev1:
            enable_files = [xx for xx in enable_files if xx not in self.dev_1_enable_files]
        else:
            self.dev_1_enable_files = enable_files
            
        is_ok = all(['0' in open(filename).read() for filename in enable_files])
        if (not is_ok):
            print ('At least 1 module\'s power is still On. Test Failed.')
            print ('Device enable files are:')
            for filename in enable_files:
                print (filename + ' : ' + open(filename).read().strip())
        return is_ok

    def frame_callback(self, dev, frame):
        profile = frame.get_profile()
        sn = dev.get_info(rs.camera_info.serial_number)
        stream_type = str(profile.stream_type())
        if self.frame_count[sn][stream_type]['start_time'] is None:
            self.frame_count[sn][stream_type]['start_time'] = time.time()
        self.frame_count[sn][stream_type]['count']+=1
        # print('%s:%s:%d' % (sn, stream_type, self.frame_count[sn][stream_type]['count']))

    def start_sensors(self, dev):
        wanted_stream_types = [rs.stream.gyro, rs.stream.accel]
        sn = dev.get_info(rs.camera_info.serial_number)
        self.frame_count[sn] = {}
        sensor = dev.first_motion_sensor()
            
        targets = []
        for profile in sensor.profiles:
            for wanted_stream_type in wanted_stream_types:
                if profile.stream_type() == wanted_stream_type:
                    targets.append(profile)
                    wanted_stream_types.remove(wanted_stream_type)
                    self.frame_count[sn][str(wanted_stream_type)] = {'count': 0, 
                                                                     'fps': profile.fps(),
                                                                     'start_time': None}

                    break
        print ('Open %d profiles.' % len(targets))
        sensor.open(targets)
        print ('Start sensor')
        sensor.start(partial(self.frame_callback, dev))

    def stop_sensors(self):
        for dev in self.open_devices:
            sensor = dev.first_motion_sensor()
            sensor.stop()
            sensor.close()

    def test_collected_frames(self, devices):
        sleep_time = 2
        print ('Collecting frames for %d(secs)' % sleep_time)
        time.sleep(sleep_time)
        print ('Done collecting frames')
        stop_time = time.time()
        for dev in devices:
            sn = dev.get_info(rs.camera_info.serial_number)
            failed_streams = [count[0] for count in self.frame_count[sn].items() if count[1]['count']<(stop_time-count[1]['start_time']-1)*count[1]['fps']]
            if (len(failed_streams) > 0):
                print ('The following streams failed to gather frames (dev: %s): %s' % (sn, ', '.join(failed_streams)))
                for count in self.frame_count[sn].items():
                    if count[0] in failed_streams:
                        print ('%s: Collected for %.2f(sec) with %d(fps). Expected %d frames. Got %d frames only.' % (count[0], 
                                                                                                                      stop_time-count[1]['start_time'], 
                                                                                                                      count[1]['fps'], count[1]['count'], 
                                                                                                                      (stop_time-count[1]['start_time']-1)*count[1]['fps'] ) )
                print self.frame_count[sn]
                return False
        return True

    def start(self, test_2_devices=True):
        # check power off for all connected devices.
        # Reset device1
        # check power off for all connected devices.
        # Start dev1
        # collect frames for N seconds from dev1
        # if test 2 cameras:
        #   connect dev2
        #   Reset device2
        #   check power off dev2
        #   Start dev2
        #   collect frames for N seconds from dev1, dev2

        # Disconnect all devices.
        # self.disconnect_all_devices()
        # Connect dev1
        self.open_devices = []
        print ('Check initial state of all connected cameras:')
        if not self.check_power_off():
            return False
        dev1 = self.connect_imu_device([])
        if (dev1 is None):
            print ('motion module device was not connected.')
            return False
        print ('motion module device connected: %s' % dev1.get_info(rs.camera_info.serial_number))
        dev1 = self.reset_device(dev1)
        # check power off dev1
        if not self.check_power_off():
            return False
        print ('All devices powered off.')
        
        self.start_sensors(dev1)
        self.open_devices.append(dev1)
        # Start dev1
        # collect frames for N seconds from dev1
        if not self.test_collected_frames([dev1]):
            return False
        # connect dev2
        if test_2_devices:
            dev2 = self.connect_imu_device([dev1.get_info(rs.camera_info.serial_number)])
            if (dev2 is None):
                print ('motion module device 2 was not connected.')
                return True
            dev2 = self.reset_device(dev2)
            # check power off dev2
            if not self.check_power_off(ignore_dev1=True):
                return False
            print ('device2 (%s) power off.' % dev2.get_info(rs.camera_info.serial_number))
            # Start dev2
            self.start_sensors(dev2)
            self.open_devices.append(dev2)
            # collect frames for N seconds from dev1, dev2
            if not self.test_collected_frames([dev1, dev2]):
                return False
        return True

def main():
    testobj = udevTester()
    testobj.message()
    test_2_devices = '--two_devices' in sys.argv

    ok = testobj.start(test_2_devices=test_2_devices)
    testobj.stop_sensors()
    if not ok:
        print ('Test Failed.')
        return -1
    print ('Test Succeeded')
    return 0


if __name__ == '__main__':
    sys.exit(main())