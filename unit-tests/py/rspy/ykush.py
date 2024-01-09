# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

"""
Yepkit USB Switchable Hub

Pip page:
https://pypi.org/project/yepkit-pykush/
Github:
https://github.com/Yepkit/pykush
"""


from rspy import log
import time

from rspy import usb_relay

if __name__ == '__main__':
    import os, sys, getopt
    def usage():
        ourname = os.path.basename( sys.argv[0] )
        print( 'Syntax: ykush [options]' )
        print( '        Control the YKUSH USB hub' )
        print( 'Options:' )
        print( '        --enable       Enable all ports' )
        print( '        --disable      Disable all ports' )
        print( '        --recycle      Recycle all ports' )
        print( '        --reset        Reset the YKUSH' )
        sys.exit(2)
    try:
        opts,args = getopt.getopt( sys.argv[1:], '',
            longopts = [ 'help', 'recycle', 'enable', 'disable', 'reset' ])
    except getopt.GetoptError as err:
        print( '-F-', err )   # something like "option -a not recognized"
        usage()
    if args or not opts:
        usage()
    # See the end of the file for all the option handling

# we assume that if pykush is not installed, it is not in use
try:
    import pykush
except ModuleNotFoundError:
    log.w( 'No pykush library is available!' )
    raise


class NoneFoundError(pykush.YKUSHNotFound):
    def __init__(self, message=None):
        super().__init__(self, message or 'no YKUSH module found')


class Ykush(usb_relay.usb_relay):
    _ykush = None
    NUM_PORTS = 3

    def __init__(self):
        super().__init__()
        yk = self.discover()
        if yk == None:
            raise NoneFoundError()
        self._ykush = None

    def discover(self, retries = 0, serial = None, path = None):
        """
        Return a YKUSH device. Raise YKUSHNotFound if none found
        If it was called more than once when there's only one device connected, return it
        """
        ykush_dev = None
        for i in range(retries + 1):
            try:
                ykush_dev = pykush.YKUSH(serial=serial, path=path)
            except pykush.YKUSHNotFound as e:
                log.w("YKUSH device not found!")
            except Exception as e:
                log.w("Unexpected error occurred!", e)
            finally:
                if not ykush_dev and i < retries:
                    time.sleep(1)
                else:
                    break
        return ykush_dev

    def connect(self, reset = False, serial = None, path = None):
        """
        Connect to a YKUSH device
        :param reset: When true, the YKUSH will be reset as part of the connection process
        """
        if reset:
            log.d("resetting YKUSH...")
            import subprocess
            subprocess.run("ykushcmd ykush3 --reset".split())
            time.sleep(5)

        if not self._ykush:
            self._ykush = self.discover(serial=serial, path=path)

        self._set_connected(self, usb_relay.YKUSH)

    def is_connected(self):
        return self._ykush is not None

    def disconnect(self):
        if self._ykush:
            del self._ykush
            self._ykush = None

    def all_ports(self):
        """
        :return: a list of all possible ports, even if currently unoccupied or disabled
        """
        return range(1,self.NUM_PORTS+1)

    def ports(self):
        """
        :return: a list of all ports currently enabled
        """
        occupied_ports = []
        for port in self.all_ports():
            if self._ykush.get_port_state(port):
                occupied_ports.append( port )
        return occupied_ports

    def is_port_enabled( self, port ):
        """
        query if the input port number is enabled through YKUSH
        :param port: port number;
        :return: True if YKUSH enabled this port, False otherwise
        """
        return self.port_state(port) == "Enabled"

    def port_state( self, port ):
        if port < 1 or port > self.NUM_PORTS:
            raise ValueError( "port number must be [1-{}]".format(self.NUM_PORTS) )
        if not self._ykush:
            raise NoneFoundError("No YKUSH found")
        return "Enabled" if self._ykush.get_port_state(port) else "Disabled"

    def enable_ports( self,  ports = None, disable_other_ports = False, sleep_on_change = 0 ):
        """
        Set enable state to provided ports
        :param ports: List of port numbers; if not provided, enable all ports
        :param disable_other_ports: if True, the ports not in the list will be disabled
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        result = True
        changed = False
        for port in self.all_ports():
            if ports is None or port in ports:
                if not self.is_port_enabled( port ):
                    action_result = self._ykush.set_port_state(port, pykush.YKUSH_PORT_STATE_UP)
                    if action_result:
                        changed = True
                    else:
                        result = False
                        log.e("Failed to enable port", port)
            #
            elif disable_other_ports:
                if self.is_port_enabled( port ):
                    action_result = self._ykush.set_port_state(port, pykush.YKUSH_PORT_STATE_DOWN)
                    if action_result:
                        changed = True
                    else:
                        result = False
                        log.e("Failed to disable port", port)

        if changed and sleep_on_change:
            time.sleep(sleep_on_change)

        return result

    def disable_ports( self, ports = None, sleep_on_change = 0 ):
        """
        :param ports: List of port numbers; if not provided, disable all ports
        :param sleep_on_change: Number of seconds to sleep if any change is made
        :return: True if no errors found, False otherwise
        """
        result = True
        changed = False
        for port in self.all_ports():
            if ports is None or port in ports:
                if self.is_port_enabled( port ):
                        action_result = self._ykush.set_port_state(port,pykush.YKUSH_PORT_STATE_DOWN)
                        if action_result:
                            changed = True
                        else:
                            result = False
                            log.e("Failed to disable port", port)

        if changed and sleep_on_change:
            time.sleep(sleep_on_change)

        return result

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
        time.sleep( timeout )
        #
        result = self.enable_ports( portlist ) and result
        #
        return result

    def get_port_from_usb(self, first_usb_index, second_usb_index ):
        """
        Based on last two USB location index, provide the port number
        On YKUSH, we only have one USB location index
        """
        ykush_port_usb_map = {1: 3,
                              2: 2,
                              3: 1}
        return ykush_port_usb_map[second_usb_index]

    def find_all_hubs(self):
        """
        Yields all hub port numbers
        """
        from rspy import lsusb
        hubs = set( lsusb.devices_by_vendor( '04d8' ))
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


if __name__ == '__main__':
    device = Ykush()
    for opt,arg in opts:
        if opt in ('--enable'):
            device.connect()
            device.enable_ports()   # so ports() will return all
        elif opt in ('--disable'):
            device.connect()
            device.disable_ports()
        elif opt in ('--recycle'):
            device.connect()
            device.enable_ports()   # so ports() will return all
            device.recycle_ports()
        elif opt in ('--reset'):
            device.connect( reset = True )
