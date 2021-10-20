# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

import sys, os, re, platform
try:
    from rspy import log
except ModuleNotFoundError:
    if __name__ != '__main__':
        raise
    #
    def usage():
        ourname = os.path.basename( sys.argv[0] )
        print( 'Syntax: devices [actions|flags]' )
        print( '        Control the LibRS devices connected' )
        print( 'Actions (only one)' )
        print( '        --list         Enumerate devices (default action)' )
        print( '        --recycle      Recycle all' )
        print( 'Flags:' )
        print( '        --all          Enable all port [requires acroname]' )
        print( '        --port <#>     Enable only this port [requires acroname]' )
        print( '        --ports        Show physical port for each device (rather than the RS string)' )
        sys.exit(2)
    #
    # We need to tell Python where to look for rspy
    rspy_dir = os.path.dirname( os.path.abspath( __file__ ))
    py_dir   = os.path.dirname( rspy_dir )
    sys.path.append( py_dir )
    from rspy import log
    #
    # And where to look for pyrealsense2
    from rspy import repo
    pyrs_dir = repo.find_pyrs_dir()
    sys.path.insert( 1, pyrs_dir )


# We need both pyrealsense2 and acroname. We can work without acroname, but
# without rs no devices at all will be returned.
try:
    import pyrealsense2 as rs
    log.d( rs )
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
_acroname_hubs = set()


class Device:
    def __init__( self, sn, dev ):
        self._sn = sn
        self._dev = dev
        self._name = None
        if dev.supports( rs.camera_info.name ):
            self._name = dev.get_info( rs.camera_info.name )
        self._product_line = None
        if dev.supports( rs.camera_info.product_line ):
            self._product_line = dev.get_info( rs.camera_info.product_line )
        self._physical_port = dev.supports( rs.camera_info.physical_port ) and dev.get_info( rs.camera_info.physical_port ) or None
        self._usb_location = None
        try:
            self._usb_location = _get_usb_location( self._physical_port )
        except Exception as e:
            log.e( 'Failed to get usb location:', e )
        self._port = None
        if acroname:
            try:
                self._port = _get_port_by_loc( self._usb_location )
            except Exception as e:
                log.e( 'Failed to get device port:', e )
                log.d( '    physical port is', self._physical_port )
                log.d( '    USB location is', self._usb_location )
        self._removed = False

    @property
    def serial_number( self ):
        return self._sn

    @property
    def name( self ):
        return self._name

    @property
    def product_line( self ):
        return self._product_line

    @property
    def physical_port( self ):
        return self._physical_port

    @property
    def usb_location( self ):
        return self._usb_location

    @property
    def port( self ):
        return self._port

    @property
    def handle( self ):
        return self._dev

    @property
    def enabled( self ):
        return self._removed is False


def wait_until_all_ports_disabled( timeout = 5 ):
    """
    Waits for all ports to be disabled
    """
    for retry in range( timeout ):
        if len( enabled() ) == 0:
            return True
        time.sleep( 1 )
    log.w( 'Timed out waiting for 0 devices' )
    return False


def map_unknown_ports():
    """
    Fill in unknown ports in devices by enabling one port at a time, finding out which device
    is there.
    """
    if not acroname:
        return
    global _device_by_sn
    devices_with_unknown_ports = [device for device in _device_by_sn.values() if device.port is None]
    if not devices_with_unknown_ports:
        return
    #
    ports = acroname.ports()
    known_ports = [device.port for device in _device_by_sn.values() if device.port is not None]
    unknown_ports = [port for port in ports if port not in known_ports]
    try:
        log.d( 'mapping unknown ports', unknown_ports, '...' )
        log.debug_indent()
        #log.d( "active ports:", ports )
        #log.d( "- known ports:", known_ports )
        #log.d( "= unknown ports:", unknown_ports )
        #
        for known_port in known_ports:
            if known_port not in ports:
                log.e( "A device was found on port", known_port, "but the port is not reported as used by Acroname!" )
        #
        if len( unknown_ports ) == 1:
            device = devices_with_unknown_ports[0]
            log.d( '... port', unknown_ports[0], 'has', device.handle )
            device._port = unknown_ports[0]
            return
        #
        acroname.disable_ports( ports )
        wait_until_all_ports_disabled()
        #
        # Enable one port at a time to try and find what device is connected to it
        n_identified_ports = 0
        for port in unknown_ports:
            #
            log.d( 'enabling port', port )
            acroname.enable_ports( [port], disable_other_ports=True )
            sn = None
            for retry in range( 5 ):
                if len( enabled() ) == 1:
                    sn = list( enabled() )[0]
                    break
                time.sleep( 1 )
            if not sn:
                log.d( 'could not recognize device in port', port )
            else:
                device = _device_by_sn.get( sn )
                if device:
                    log.d( '... port', port, 'has', device.handle )
                    device._port = port
                    n_identified_ports += 1
                    if len( devices_with_unknown_ports ) == n_identified_ports:
                        #log.d( 'no more devices; stopping' )
                        break
                else:
                    log.w( "Device with serial number", sn, "was found in port", port,
                            "but was not in context" )
            acroname.disable_ports( [port] )
            wait_until_all_ports_disabled()
    finally:
        log.debug_unindent()


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
        if not acroname.hub:
            acroname.connect()  # MAY THROW!
            acroname.enable_ports( sleep_on_change = 5 )  # make sure all connected!
            if platform.system() == 'Linux':
                global _acroname_hubs
                _acroname_hubs = set( acroname.find_all_hubs() )
    #
    # Get all devices, and store by serial-number
    global _device_by_sn, _context, _port_to_sn
    _context = rs.context()
    _device_by_sn = dict()
    try:
        log.d( 'discovering devices ...' )
        log.debug_indent()
        for retry in range(3):
            try:
                devices = _context.query_devices()
                break
            except RuntimeError as e:
                log.d( 'FAILED to query devices:', e )
                if retry > 1:
                    log.e( 'FAILED to query devices', retry + 1, 'times!' )
                    raise
                else:
                    time.sleep( 1 )
        for dev in devices:
            # The FW update ID is always available, it seems, and is the ASIC serial number
            # whereas the Serial Number is the OPTIC serial number and is only available in
            # non-recovery devices. So we use the former...
            sn = dev.get_info( rs.camera_info.firmware_update_id )
            device = Device( sn, dev )
            _device_by_sn[sn] = device
            log.d( '... port {}:'.format( device.port is None and '?' or device.port ), sn, dev )
    finally:
        log.debug_unindent()
    #
    if monitor_changes:
        _context.set_devices_changed_callback( _device_change_callback )


def _device_change_callback( info ):
    """
    Called when librealsense detects a device change (see query())
    """
    global _device_by_sn
    for device in _device_by_sn.values():
        if device.enabled  and  info.was_removed( device.handle ):
            log.d( 'device removed:', device.serial_number )
            device._removed = True
    for handle in info.get_new_devices():
        sn = handle.get_info( rs.camera_info.firmware_update_id )
        log.d( 'device added:', sn, handle )
        if sn in _device_by_sn:
            device = _device_by_sn[sn]
            device._removed = False
            device._dev = handle     # Because it has a new handle!
        else:
            # shouldn't see new devices...
            log.d( 'new device detected!?' )
            _device_by_sn[sn] = Device( sn, handle )


def all():
    """
    :return: A set of all device serial-numbers at the time of query()
    """
    global _device_by_sn
    return _device_by_sn.keys()


def enabled():
    """
    :return: A set of all device serial-numbers that are currently enabled
    """
    global _device_by_sn
    return { device.serial_number for device in _device_by_sn.values() if device.enabled }


def by_product_line( product_line ):
    """
    :param product_line: The product line we're interested in, as a string ("L500", etc.)
    :return: A set of device serial-numbers
    """
    global _device_by_sn
    return { device.serial_number for device in _device_by_sn.values() if device.product_line == product_line }


def by_name( name ):
    """
    :param name: Part of the product name to search for ("L515" would match "Intel RealSense L515")
    :return: A set of device serial-numbers
    """
    global _device_by_sn
    return { device.serial_number for device in _device_by_sn.values() if device.name  and  device.name.find( name ) >= 0 }


def _get_sns_from_spec( spec ):
    """
    Helper function for by_configuration. Yields all serial-numbers matching the given spec
    """
    if spec.endswith( '*' ):
        for sn in by_product_line( spec[:-1] ):
            yield sn
    else:
        for sn in by_name( spec ):
            yield sn


def expand_specs( specs ):
    """
    Given a collection of configuration specs, expand them into actual serial numbers.
    Specs can be loaded from a file: see load_specs_from_file()
    :param specs: a collection of specs
    :return: a set of serial-numbers
    """
    expanded = set()
    for spec in specs:
        sns = {sn for sn in _get_sns_from_spec( spec )}
        if sns:
            expanded.update( sns )
        else:
            # maybe the spec is a specific serial-number?
            if get(spec):
                expanded.add( spec )
            else:
                log.d( 'unknown spec:', spec )
    return expanded


def load_specs_from_file( filename ):
    """
    Loads a set of specs from a file:
        - Comments (#) are removed
        - Each word in the file is a spec
    :param filename: the path to the text file we want to load
    :return: a set of specs that can then be expanded to a set of serial-numbers (see expand_specs())
    """
    from rspy import file
    exceptions = set()
    for line, comment in file.split_comments( filename ):
        specs = line.split()
        if specs:
            log.d( '...', specs, comment and ('  # ' + comment) or '', )
            exceptions.update( specs )
    return exceptions


def by_configuration( config, exceptions = None ):
    """
    Yields the serial numbers fitting the given configuration. If configuration includes an 'each' directive
    will yield all fitting serial numbers one at a time. Otherwise yields one set of serial numbers fitting the configuration

    :param config: A test:device line collection of arguments (e.g., [L515 D400*])
    :param exceptions: A collection of serial-numbers that serve as exceptions that will never get matched

    If no device matches the configuration devices specified, a RuntimeError will be
    raised!
    """
    exceptions = exceptions or set()
    if len( config ) == 1 and re.fullmatch( r'each\(.+\)', config[0], re.IGNORECASE ):
        spec = config[0][5:-1]
        for sn in _get_sns_from_spec( spec ):
            if sn not in exceptions:
                yield { sn }
    else:
        sns = set()
        for spec in config:
            old_len = len(sns)
            for sn in _get_sns_from_spec( spec ):
                if sn in exceptions:
                    continue
                if sn not in sns:
                    sns.add( sn )
                    break
            new_len = len(sns)
            if new_len == old_len:
                error = 'no device matches configuration "' + spec + '"'
                if old_len:
                    error += ' (after already matching ' + str(sns) + ')'
                if exceptions:
                    error += ' (-' + str(exceptions) + ')'
                raise RuntimeError( error )
        if sns:
            yield sns


def get_first( sns ):
    """
    Throws if no serial-numbers are available!
    :param sns: An iterable list of serial-numbers. If None, defaults to all enabled() devices
    :return: The first Device in the given serial-numbers
    """
    return get( next( iter( sns or enabled() )))


def get( sn ):
    """
    :param sn: The serial-number of the requested device
    :return: The Device object with the given SN, or None
    """
    global _device_by_sn
    return _device_by_sn.get(sn)


def get_by_port( port ):
    """
    Return the Device at the given port number. Note that the device may not be enabled!
    :param sn: The port of the requested device
    :return: The Device object, or None
    """
    global _device_by_sn
    for sn in all():
        device = get( sn )
        if device.port == port:
            return device
    return None


def recovery():
    """
    :return: A set of all device serial-numbers that are in recovery mode
    """
    global _device_by_sn
    return { device.serial_number for device in _device_by_sn.values() if device.handle.is_update_device() }


def enable_only( serial_numbers, recycle = False, timeout = 5 ):
    """
    Enable only the devices corresponding to the given serial-numbers. This can work either
    with or without Acroname: without, the devices will simply be HW-reset, but other devices
    will still be present.

    NOTE: will raise an exception if any SN is unknown!

    :param serial_numbers: A collection of serial-numbers to enable - all others' ports are
                           disabled and will no longer be usable!
    :param recycle: If False, the devices will not be reset if they were already enabled. If
                    True, the devices will be recycled by disabling the port, waiting, then
                    re-enabling
    :param timeout: The maximum seconds to wait to make sure the devices are indeed online
    """
    if acroname:
        #
        ports = [ get( sn ).port for sn in serial_numbers ]
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
    Enables all ports on an Acroname -- without an Acroname, this does nothing!
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
    while True:
        have_devices = False
        enabled_sns = enabled()
        for sn in serial_numbers:
            if sn in enabled_sns:
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
    did_some_waiting = False
    #
    # In Linux, we don't have an active notification mechanism - we query devices every 5 seconds
    # (see POLLING_DEVICES_INTERVAL_MS) - so we add extra timeout
    if timeout and platform.system() == 'Linux':
        timeout += 5
    #
    while True:
        #
        have_all_devices = True
        enabled_sns = enabled()
        for sn in serial_numbers:
            if sn not in enabled_sns:
                have_all_devices = False
                break
        #
        if have_all_devices:
            if did_some_waiting:
                # Wait an extra second, just in case -- let the devices properly power up
                #log.d( 'all devices powered up' )
                time.sleep( 1 )
            return True
        #
        if timeout <= 0:
            if did_some_waiting:
                log.d( 'timed out' )
            return False
        timeout -= 1
        time.sleep( 1 )
        did_some_waiting = True


def hw_reset( serial_numbers, timeout = 5 ):
    """
    Recycles the given devices manually, using a hardware-reset (rather than any acroname port
    reset). The devices are sent a HW-reset command and then we'll wait until they come back
    online.

    NOTE: will raise an exception if any SN is unknown!

    :param serial_numbers: A collection of serial-numbers to reset
    :param timeout: Maximum # of seconds to wait for the devices to come back online
    :return: True if all devices have come back online before timeout
    """
    for sn in serial_numbers:
        dev = get( sn ).handle
        dev.hardware_reset()
    #
    _wait_until_removed( serial_numbers )
    #
    return _wait_for( serial_numbers, timeout = timeout )


###############################################################################################
import platform
if 'windows' in platform.system().lower():
    #
    def _get_usb_location( physical_port ):
        """
        Helper method to get windows USB location from registry
        """
        if not physical_port:
            return None
        # physical port example:
        #   \\?\usb#vid_8086&pid_0b07&mi_00#6&8bfcab3&0&0000#{e5323777-f976-4f5b-9b55-b94699c46e44}\global
        #
        re_result = re.match( r'.*\\(.*)#vid_(.*)&pid_(.*)(?:&mi_(.*))?#(.*)#', physical_port, flags = re.IGNORECASE )
        dev_type = re_result.group(1)
        vid = re_result.group(2)
        pid = re_result.group(3)
        mi = re_result.group(4)
        unique_identifier = re_result.group(5)
        #
        import winreg
        if mi:
            registry_path = "SYSTEM\CurrentControlSet\Enum\{}\VID_{}&PID_{}&MI_{}\{}".format(
                dev_type, vid, pid, mi, unique_identifier
                )
        else:
            registry_path = "SYSTEM\CurrentControlSet\Enum\{}\VID_{}&PID_{}\{}".format(
                dev_type, vid, pid, unique_identifier
                )
        try:
            reg_key = winreg.OpenKey( winreg.HKEY_LOCAL_MACHINE, registry_path )
        except FileNotFoundError:
            log.e( 'Could not find registry key for port:', registry_path )
            log.e( '    usb location:', physical_port )
            return None
        result = winreg.QueryValueEx( reg_key, "LocationInformation" )
        # location example: 0000.0014.0000.016.003.004.003.000.000
        # and, for T265: Port_#0002.Hub_#0006
        return result[0]
    #
    def _get_port_by_loc( usb_location ):
        """
        """
        if usb_location:
            #
            # T265 locations look differently...
            match = re.fullmatch( r'Port_#(\d+)\.Hub_#(\d+)', usb_location, re.IGNORECASE )
            if match:
                # We don't know how to get the port from these yet!
                return None #int(match.group(2))
            else:
                split_location = [int(x) for x in usb_location.split('.')]
                # only the last two digits are necessary
                return acroname.get_port_from_usb( split_location[-5], split_location[-4] )
    #
else:
    #
    def _get_usb_location( physical_port ):
        """
        """
        if not physical_port:
            return None
        # physical port example:
        # u'/sys/devices/pci0000:00/0000:00:14.0/usb2/2-3/2-3.3/2-3.3.1/2-3.3.1:1.0/video4linux/video0'
        #
        split_location = physical_port.split( '/' )
        if len(split_location) > 4:
            port_location = split_location[-4]
        elif len(split_location) == 1:
            # Recovery devices may have only the relevant port: e.g., L515 Recovery has "2-2.4.4-84"
            port_location = physical_port
        else:
            raise RuntimeError( f"invalid physical port '{physical_port}'" )
        # location example: 2-3.3.1
        return port_location
    #
    def _get_port_by_loc( usb_location ):
        """
        """
        if usb_location:
            #
            # Devices connected thru an acroname will be in one of two sub-hubs under the acroname main
            # hub. Each is a 4-port hub with a different port (4 for ports 0-3, 3 for ports 4-7):
            #     /:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/6p, 10000M
            #         |__ Port 2: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M                       <--- ACRONAME
            #             |__ Port 3: Dev 3, If 0, Class=Hub, Driver=hub/4p, 5000M
            #                 |__ Port X: Dev, If...
            #                 |__ Port Y: ...
            #             |__ Port 4: Dev 4, If 0, Class=Hub, Driver=hub/4p, 5000M
            #                 |__ Port Z: ...
            # (above is output from 'lsusb -t')
            # For the above acroname at '2-2' (bus 2, port 2), there are at least 3 devices:
            #     2-2.3.X
            #     2-2.3.Y
            #     2-2.4.Z
            # Given the two sub-ports (3.X, 3.Y, 4.Z), we can get the port number.
            # NOTE: some of our devices are hubs themselves! For example, the SR300 will show as '2-2.3.2.1' --
            # we must start a known hub or else the ports we look at are meaningless...
            #
            global _acroname_hubs
            for port in _acroname_hubs:
                if usb_location.startswith( port + '.' ):
                    match = re.search( r'^(\d+)\.(\d+)', usb_location[len(port)+1:] )
                    if match:
                        return acroname.get_port_from_usb( int(match.group(1)), int(match.group(2)) )


###############################################################################################
if __name__ == '__main__':
    import os, sys, getopt
    try:
        opts,args = getopt.getopt( sys.argv[1:], '',
            longopts = [ 'help', 'recycle', 'all', 'list', 'port=', 'ports' ])
    except getopt.GetoptError as err:
        print( '-F-', err )   # something like "option -a not recognized"
        usage()
    if args:
        usage()
    try:
        if acroname:
            if not acroname.hub:
                acroname.connect()
                if platform.system() == 'Linux':
                    _acroname_hubs = set( acroname.find_all_hubs() )
        action = 'list'
        def get_handle(dev):
            return dev.handle
        def get_phys_port(dev):
            return dev.physical_port or "???"
        printer = get_handle
        for opt,arg in opts:
            if opt in ('--list'):
                action = 'list'
            elif opt in ('--ports'):
                printer = get_phys_port
            elif opt in ('--all'):
                if not acroname:
                    log.f( 'No acroname available' )
                acroname.enable_ports( sleep_on_change = 5 )
            elif opt in ('--port'):
                if not acroname:
                    log.f( 'No acroname available' )
                all_ports = acroname.all_ports()
                str_ports = arg.split(',')
                ports = [int(port) for port in str_ports if port.isnumeric() and int(port) in all_ports]
                if len(ports) != len(str_ports):
                    log.f( 'Invalid ports', str_ports )
                acroname.enable_ports( ports, disable_other_ports = True, sleep_on_change = 5 )
            elif opt in ('--recycle'):
                action = 'recycle'
            else:
                usage()
        if action == 'list':
            query()
            for sn in all():
                device = get( sn )
                print( '{port} {name:30} {sn:20} {handle}'.format(
                    sn = sn,
                    name = device.name,
                    port = device.port is None and '?' or device.port,
                    handle = printer(device)
                    ))
        elif action == 'recycle':
            log.f( 'Not implemented yet' )
    finally:
        #
        # Disconnect from the Acroname -- if we don't it'll crash on Linux...
        if acroname:
            acroname.disconnect()


