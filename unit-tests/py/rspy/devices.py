# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

from rspy import log
import sys, os

# We need both pyrealsense2 and acroname. We can work without acroname, but
# without rs no devices at all will be returned.
try:
    import pyrealsense2 as rs
    log.d( rs )
    #
    # Have to add site-packages, just in case: if -S was used, or parent script played with
    # sys.path (as run-unit-tests does), then we may not have it!
    sys.path += [os.path.join( os.path.dirname( sys.executable ), 'lib', 'site-packages')]
    #
    try:
        from rspy import acroname
    except ModuleNotFoundError:
        # Error should have already been printed
        # We assume there's no brainstem library, meaning no acroname either
        log.d( 'sys.path=', sys.path )
        acroname = None
    #
    sys.path = sys.path[:-1]  # remove what we added
except ModuleNotFoundError:
    log.w( 'No pyrealsense2 library is available! Running as if no cameras available...' )
    import sys
    log.d( 'sys.path=', sys.path )
    rs = None
    acroname = None

import time

_device_by_sn = dict()
_context = None


def query( monitor_changes = True ):
    """
    Start a new LRS context, and collect all devices
    :param monitor_changes: If True, devices will update dynamically as they are removed/added
    """
    global rs
    if not rs:
        return
    #
    # Before we can start a context and query devices, we need to enable all the ports
    # on the acroname, if any:
    if acroname:
        acroname.connect()  # MAY THROW!
        acroname.enable_ports()
    #
    # Get all devices, and store by serial-number
    global _device_by_sn, _context, _port_to_sn
    _context = rs.context()
    if monitor_changes:
        _context.set_devices_changed_callback( _device_change_callback )
    _device_by_sn = dict()
    for dev in _context.query_devices():
        if dev.is_update_device():
            sn = dev.get_info( rs.camera_info.firmware_update_id )
        else:
            sn = dev.get_info( rs.camera_info.serial_number )
        _device_by_sn[sn] = dev


def _device_change_callback( info ):
    """
    Called when librealsense detects a device change (see query())
    """
    global _device_by_sn
    new_device_by_sn = dict()
    for sn, dev in _device_by_sn.items():
        if info.was_removed( dev ):
            log.d( 'device removed:', sn )
        else:
            new_device_by_sn[sn] = dev
    for dev in info.get_new_devices():
        if dev.is_update_device():
            sn = dev.get_info( rs.camera_info.firmware_update_id )
        else:
            sn = dev.get_info( rs.camera_info.serial_number )
        log.d( 'device added:', dev )
        new_device_by_sn[sn] = dev
    _device_by_sn = new_device_by_sn


def all():
    """
    :return: A list of all device serial-numbers
    """
    global _device_by_sn
    return _device_by_sn.keys()


def by_product_line( product_line ):
    """
    :param product_line: The product line we're interested in, as a string ("L500", etc.)
    :return: A list of device serial-numbers
    """
    sns = set()
    global _device_by_sn
    for sn, dev in _device_by_sn.items():
        if dev.supports( rs.camera_info.product_line ) and dev.get_info( rs.camera_info.product_line ) == product_line:
            sns.add( sn )
    return sns


def by_name( name ):
    """
    :param name: Part of the product name to search for ("L515" would match "Intel RealSense L515")
    :return: A list of device serial-numbers
    """
    sns = set()
    global _device_by_sn
    for sn, dev in _device_by_sn.items():
        if dev.supports( rs.camera_info.name ) and dev.get_info( rs.camera_info.name ).find( name ) >= 0:
            sns.add( sn )
    return sns


def by_configuration( config ):
    """
    :param config: A test:device line collection of arguments (e.g., [L515 D400*])
    :return: A set of device serial-numbers matching

    If no device matches the configuration devices specified, a RuntimeError will be
    raised!
    """
    sns = set()
    for spec in config:
        old_len = len(sns)
        if spec.endswith( '*' ):
            # By product line
            for sn in by_product_line( spec[:-1] ):
                if sn not in sns:
                    sns.add( sn )
                    break
        else:
            # By name
            for sn in by_name( spec ):
                if sn not in sns:
                    sns.add( sn )
                    break
        new_len = len(sns)
        if new_len == old_len:
            if old_len:
                raise RuntimeError( 'no device matches configuration "' + spec + '" (after already matching ' + str(sns) + ')' )
            else:
                raise RuntimeError( 'no device matches configuration "' + spec + '"' )
    return sns


def get( sn ):
    """
    :param sn: The serial-number of the requested device
    :return: The pyrealsense2.device object with the given SN, or None
    """
    global _device_by_sn
    return _device_by_sn.get(sn)


def get_port( sn ):
    """
    :param sn: The serial-number of the requested device
    :return: The port number (0-7) the device is on
    """
    return _get_port_by_dev( get( sn ))


def enable_only( serial_numbers, recycle = False, timeout = 5 ):
    """
    Enable only the devices corresponding to the given serial-numbers. This can work either
    with or without Acroname: without, the devices will simply be HW-reset, but other devices
    will still be present.

    :param serial_numbers: A collection of serial-numbers to enable - all others' ports are
                           disabled and will no longer be usable!
    :param recycle: If False, the devices will not be reset if they were already enabled. If
                    True, the devices will be recycled by disabling the port, waiting, then
                    re-enabling
    :param timeout: The maximum seconds to wait to make sure the devices are indeed online
    """
    if acroname:
        #
        ports = [ get_port( sn ) for sn in serial_numbers ]
        #
        if recycle:
            #
            log.d( 'recycling ports via acroname:', ports )
            #
            acroname.disable_ports( acroname.ports() )
            _wait_until_removed( serial_numbers, timeout = timeout )
            #
            acroname.enable_ports( ports )
            #
        else:
            #
            acroname.enable_ports( ports, disable_other_ports = True )
        #
        _wait_for( serial_numbers, timeout = timeout )
        #
    elif recycle:
        #
        hw_reset( serial_numbers )
        #
    else:
        log.d( 'no acroname; ports left as-is' )


def enable_all():
    """
    """
    if acroname:
        acroname.enable_ports()


def _wait_until_removed( serial_numbers, timeout = 5 ):
    """
    Wait until the given serial numbers are all offline

    :param serial_numbers: A collection of serial-numbers to wait until removed
    :param timeout: Number of seconds of maximum wait time
    :return: True if all have come offline; False if timeout was reached
    """
    global _device_by_sn
    while True:
        have_devices = False
        for sn in serial_numbers:
            if sn in _device_by_sn:
                have_devices = True
                break
        if not have_devices:
            return True
        #
        if timeout <= 0:
            return False
        timeout -= 1
        time.sleep( 1 )


def _wait_for( serial_numbers, timeout = 5 ):
    """
    Wait until the given serial numbers are all online

    :param serial_numbers: A collection of serial-numbers to wait for
    :param timeout: Number of seconds of maximum wait time
    :return: True if all have come online; False if timeout was reached
    """
    global _device_by_sn
    did_some_waiting = False
    while True:
        #
        have_all_devices = True
        for sn in serial_numbers:
            if sn not in _device_by_sn:
                have_all_devices = False
                break
        #
        if have_all_devices:
            if did_some_waiting:
                # Wait an extra second, just in case -- let the devices properly power up
                log.d( 'all devices powered up' )
                time.sleep( 1 )
            return True
        #
        if timeout <= 0:
            return False
        timeout -= 1
        time.sleep( 1 )
        did_some_waiting = True


def hw_reset( serial_numbers, timeout = 5 ):
    """
    Recycles the given devices manually, using a hardware-reset (rather than any acroname port
    reset). The devices are sent a HW-reset command and then we'll wait until they come back
    online.

    :param serial_numbers: A collection of serial-numbers to reset
    :param timeout: Maximum # of seconds to wait for the devices to come back online
    :return: True if all devices have come back online before timeout
    """
    for sn in serial_numbers:
        dev = get( sn )
        dev.hardware_reset()
    #
    _wait_until_removed( serial_numbers )
    #
    return _wait_for( serial_numbers, timeout = timeout )


###############################################################################################
import platform
if 'windows' in platform.system().lower():
    #
    def _get_usb_location( dev ):
        """
        Helper method to get windows USB location from registry
        """
        if not dev.supports( rs.camera_info.physical_port ):
            return None
        usb_location = dev.get_info( rs.camera_info.physical_port )
        # location example:
        # u'\\\\?\\usb#vid_8086&pid_0b07&mi_00#6&8bfcab3&0&0000#{e5323777-f976-4f5b-9b55-b94699c46e44}\\global'
        #
        import re
        re_result = re.match( r'.*\\(.*)#vid_(.*)&pid_(.*)&mi_(.*)#(.*)#', usb_location )
        dev_type = re_result.group(1)
        vid = re_result.group(2)
        pid = re_result.group(3)
        mi = re_result.group(4)
        unique_identifier = re_result.group(5)
        #
        import winreg
        registry_path = "SYSTEM\CurrentControlSet\Enum\{}\VID_{}&PID_{}&MI_{}\{}".format(
            dev_type, vid, pid, mi, unique_identifier
            )
        reg_key = winreg.OpenKey( winreg.HKEY_LOCAL_MACHINE, registry_path )
        result = winreg.QueryValueEx( reg_key, "LocationInformation" )
        # location example: 0000.0014.0000.016.003.004.003.000.000
        return result[0]
    #
    def _get_port_by_dev( dev ):
        """
        """
        usb_location = _get_usb_location( dev )
        if usb_location:
            split_location = [int(x) for x in usb_location.split('.') if int(x) > 0]
            # only the last two digits are necessary
            return acroname.get_port_from_usb( split_location[-2], split_location[-1] )
    #
else:
    #
    def _get_usb_location( dev ):
        """
        """
        if not dev.supports( rs.camera_info.physical_port ):
            return None
        usb_location = dev.get_info( rs.camera_info.physical_port )
        # location example:
        # u'/sys/devices/pci0000:00/0000:00:14.0/usb2/2-3/2-3.3/2-3.3.1/2-3.3.1:1.0/video4linux/video0'
        #
        split_location = usb_location.split( '/' )
        port_location = split_location[-4]
        # location example: 2-3.3.1
        return port_location
    #
    def _get_port_by_dev( dev ):
        """
        """
        usb_location = _get_usb_location( dev )
        split_port_location = usb_location.split( '.' )
        first_port_coordinate = int( split_port_location[-2] )
        second_port_coordinate = int( split_port_location[-1] )
        port = acroname.get_port_from_usb( first_port_coordinate, second_port_coordinate )
        return port

