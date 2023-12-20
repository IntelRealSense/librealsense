# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import re
from rspy import log

NO_RELAY = 0
ACRONAME = 1
YKUSH = 2


class usb_relay:

    def __init__(self):
        self._relay = None
        self._relay_type = NO_RELAY
        self._is_connected = False

    def __del__(self):
        self.disable_ports()
        self.disconnect()

    def connect(self, reset = False):
        if self.is_connected():
            log.d("Already connected to", self._relay, "disconnecting...")
            self.disconnect()
        self._relay_type, self._relay = _find_active_relay()
        if self._relay:
            self._relay.connect(reset)
            self._is_connected = True
        log.d("Now connected to:", self._relay)

    def disconnect(self):
        if self._relay:
            self._relay.disconnect()
            del self._relay
            self._relay = None
            self._relay_type = NO_RELAY
            self._is_connected = False

    def is_connected(self):
        return self._is_connected

    def ports(self):
        if self._relay_type != NO_RELAY:
            return self._relay.ports()

    def all_ports(self):
        if self._relay_type != NO_RELAY:
            return self._relay.all_ports()

    def enable_ports(self, ports=None, disable_other_ports=False, sleep_on_change=0):
        """
        Set enable state to provided ports
        :param ports: List of port numbers; if not provided, enable all ports
        :param disable_other_ports: if True, the ports not in the list will be disabled
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        if self._relay_type != NO_RELAY:
            return self._relay.enable_ports(ports, disable_other_ports, sleep_on_change)
        return False

    def disable_ports(self, ports=None, sleep_on_change=0):
        """
        :param ports: List of port numbers; if not provided, disable all ports
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        if self._relay_type != NO_RELAY:
            return self._relay.disable_ports(ports, sleep_on_change)
        return False

    def recycle_ports(self, portlist=None, timeout=2):
        """
        Disable and enable a port
        :param portlist: List of port numbers; if not provided, recycle all ports
        :param timeout: how long to wait before re-enabling
        :return: True if everything OK, False otherwise
        """
        if self._relay_type != NO_RELAY:
            return self._relay.recycle_ports(portlist=portlist, timeout=timeout)
        return False

    def find_all_hubs(self):
        """
        Yields all hub port numbers
        """
        if self._relay_type != NO_RELAY:
            return self._relay.find_all_hubs()

    @property
    def has_relay(self):
        return self._relay_type != NO_RELAY

    @property
    def not_supports_port_mapping(self):
        """
        :return: whether the connected relay supports port mapping, currently only the acroname supports it
        """
        return self._relay_type != ACRONAME

    def get_usb_and_port_location(self, physical_port, hubs):
        usb_location = None
        port = None
        try:
            usb_location = _get_usb_location( physical_port )
        except Exception as e:
            log.e( 'Failed to get usb location:', e )

        if self.has_relay:
            try:
                port = _get_port_by_loc( usb_location, hubs, self._relay, self._relay_type )
            except Exception as e:
                log.e( 'Failed to get device port:', e )
                log.d( '    physical port is', physical_port )
                log.d( '    USB location is', usb_location )

        return usb_location, port


#################################
def _find_active_relay():
    """
    Function finds an available relay to connect to and returns it
    """
    acroname_relay = _is_there_acroname()
    if acroname_relay:
        return ACRONAME, acroname_relay

    ykush_relay = _is_there_ykush()
    if ykush_relay:
        return YKUSH, ykush_relay

    return NO_RELAY, None


def _is_there_acroname():
    try:
        from rspy import acroname
        return acroname.Acroname()
    except ModuleNotFoundError:
        return None
    except acroname.NoneFoundError():
        return None
    except:
        return None


def _is_there_ykush():
    try:
        from rspy import ykush
        return ykush.Ykush()
    except ModuleNotFoundError:
        return None
    except ykush.NoneFoundError:
        return None
    except:
        return None


###############################################################################################
import platform
if 'windows' in platform.system().lower():
    #
    def _get_usb_location( physical_port ):
        """
        Helper method to get Windows USB location from registry
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
    def _get_port_by_loc( usb_location, hubs, acroname_relay, relay_type):
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
                return acroname_relay.get_port_from_usb( split_location[-5], split_location[-4] )
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

    def _get_port_by_loc( usb_location, hubs, acroname_relay, relay_type ):
        """
        """
        if relay_type == YKUSH:
            return acroname_relay.get_port_from_usb(int(usb_location.split(".")[1]), 0)
        if usb_location:
            #
            # Devices connected through an acroname will be in one of two sub-hubs under the acroname main
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
            for port in hubs:
                if usb_location.startswith( port + '.' ):
                    match = re.search( r'^(\d+)\.(\d+)', usb_location[len(port)+1:] )
                    if match:
                        return acroname_relay.get_port_from_usb( int(match.group(1)), int(match.group(2)) )
