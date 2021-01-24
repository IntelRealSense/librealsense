from rspy import acroname
import pyrealsense2 as rs

_device_by_sn = None
_context = None


def query():
    """
    Start a new LRS context, and collect all devices
    """
    #
    # Before we can start a context and query devices, we need to enable all the ports
    # on the acroname, if any:
    acroname.connect()  # MAY THROW!
    acroname.enable_ports()
    #
    # Get all devices, and store by serial-number
    global _device_by_sn, _context, _port_to_sn
    _context = rs.context()
    _device_by_sn = {}
    for dev in _context.query_devices():
        if dev.is_update_device():
            sn = dev.get_info( rs.camera_info.firmware_update_id )
        else:
            sn = dev.get_info( rs.camera_info.serial_number )
        _device_by_sn[sn] = dev


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
    sns = []
    global _device_by_sn
    for sn, dev in _device_by_sn.items():
        if dev.supports( rs.camera_info.product_line ) and dev.get_info( rs.camera_info.product_line ) == product_line:
            sns.append( sn )
    return sns


def by_name( name ):
    """
    :param name: Part of the product name to search for ("L515" would match "Intel RealSense L515")
    :return: A list of device serial-numbers
    """
    sns = []
    global _device_by_sn
    for sn, dev in _device_by_sn.items():
        if dev.supports( rs.camera_info.name ) and dev.get_info( rs.camera_info.name ).find( name ) >= 0:
            sns.append( sn )
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


def enable_only( serial_numbers ):
    """
    :param serial_numbers: A collection of serial-numbers to enable - all others' ports are
                           disabled and will no longer be usable!
    """
    ports = [ get_port( sn ) for sn in serial_numbers ]
    acroname.enable_ports( ports )


def enable_all():
    """
    """
    enable_only( all() )

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

