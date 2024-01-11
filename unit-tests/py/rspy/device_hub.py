# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

import re
from rspy import log
import platform
from abc import ABC, abstractmethod


class NoneFoundError( RuntimeError ):
    def __init__( self, message = None ):
        super().__init__( self, message  or  'no hub found' )


class device_hub(ABC):
    @abstractmethod
    def connect(self, reset = False):
        """
        Connect to a hub device
        :param reset: When true, the hub will be reset as part of the connection process
        """
        pass

    @abstractmethod
    def is_connected(self):
        """
        :return: True if the hub is connected, False otherwise
        """
        pass

    @abstractmethod
    def disconnect(self):
        """
        Disconnect from the hub
        """
        pass

    @abstractmethod
    def all_ports(self):
        """
        :return: a list of all possible ports, even if currently unoccupied or disabled
        """
        pass

    @abstractmethod
    def ports(self):
        """
        :return: a list of all ports currently enabled
        """
        pass

    @abstractmethod
    def is_port_enabled( self, port ):
        """
        query if the input port number is enabled through the hub
        :param port: port number;
        :return: True if the hub enabled this port, False otherwise
        """
        pass

    @abstractmethod
    def port_state( self, port ):
        pass

    @abstractmethod
    def enable_ports( self,  ports = None, disable_other_ports = False, sleep_on_change = 0 ):
        """
        Set enable state to provided ports
        :param ports: List of port numbers; if not provided, enable all ports
        :param disable_other_ports: if True, the ports not in the list will be disabled
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        pass

    @abstractmethod
    def disable_ports( self, ports = None, sleep_on_change = 0 ):
        """
        :param ports: List of port numbers; if not provided, disable all ports
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        pass

    @abstractmethod
    def _get_port_by_loc(self, usb_location, hubs):
        """
        """
        pass

    def recycle_ports(self, portlist = None, timeout = 2 ):
        """
        Disable and enable a port
        :param timeout: how long to wait before re-enabling
        :return: True if everything OK, False otherwise
        """
        if portlist is None:
            portlist = self.ports()
        #
        result = self.disable_ports( portlist )
        #
        import time
        time.sleep( timeout )
        #
        result = self.enable_ports( portlist ) and result
        #
        return result

    def get_usb_and_port_location(self, physical_port, hubs):
        usb_location = None
        port = None
        try:
            usb_location = _get_usb_location( physical_port )
        except Exception as e:
            log.e( 'Failed to get usb location:', e )

        try:
            port = self._get_port_by_loc(usb_location, hubs)
        except Exception as e:
            log.e('Failed to get device port:', e)
            log.d('    physical port is', physical_port)
            log.d('    USB location is', usb_location)

        return usb_location, port


#################################

def find_all_hubs(vid):
    """
    Yields all hub port numbers
    """
    from rspy import lsusb
    hubs = set( lsusb.devices_by_vendor( vid ))
    ports = set()
    # go thru the tree and find only the top-level ones (which we should encounter first)
    for dev,dev_port in lsusb.tree():
        if dev not in hubs:
            continue
        for port in ports:
            # ignore everything inside existing hubs - we only want the top-level
            if dev_port.startswith( port + '.' ):
                break
        else:
            ports.add( dev_port )
            yield dev_port


def create():
    return _find_active_hub()


def _find_active_hub():
    """
    Function finds an available hub to connect to and returns it
    """
    acroname_hub = _create_acroname()
    if acroname_hub:
        return acroname_hub

    ykush_hub = _create_ykush()
    if ykush_hub:
        return ykush_hub
    import sys
    log.d('sys.path=', sys.path)
    return None


def _create_acroname():
    try:
        from rspy import acroname
        return acroname.Acroname()
    except ModuleNotFoundError:
        return None
    except acroname.NoneFoundError():
        return None
    except:
        return None


def _create_ykush():
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
if 'windows' in platform.system().lower():
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
else:
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
