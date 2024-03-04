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
    def get_port_by_location(self, usb_location):
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


#################################

def find_all_hubs(vid):
    """
    Yields all hub port numbers
    """
    # example for Acroname hub:
    # 24ff:8013 =
    #   iManufacturer           Acroname Inc.
    #   iProduct                USBHub3p-3[A]
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
    except acroname.NoneFoundError:
        return None
    except BaseException:
        return None


def _create_ykush():
    try:
        from rspy import ykush
        return ykush.Ykush()
    except ModuleNotFoundError:
        return None
    except ykush.NoneFoundError:
        return None
    except BaseException:
        return None
