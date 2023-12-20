# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

"""
Brainstem Acroname Hub

See documentation for brainstem here:
https://acroname.com/reference/api/python/index.html
"""

from rspy import log
import time


if __name__ == '__main__':
    import os, sys, getopt
    def usage():
        ourname = os.path.basename( sys.argv[0] )
        print( 'Syntax: acroname [options]' )
        print( '        Control the acroname USB hub' )
        print( 'Options:' )
        print( '        --enable       Enable all ports' )
        print( '        --disable      Disable all ports' )
        print( '        --recycle      Recycle all ports' )
        print( '        --reset        Reset the acroname' )
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


try:
    import brainstem
except ModuleNotFoundError:
    log.w( 'No acroname library is available!' )
    raise

class NoneFoundError( RuntimeError ):
    def __init__( self, message = None ):
        super().__init__( self, message  or  'no Acroname module found' )

class Acroname:

    hub = None

    def __init__(self):
        self.discover()

    def __del__(self):
        self.disconnect()


    def discover(self, retries = 0):
        """
        Return all Acroname module specs in a list. Raise NoneFoundError if one is not found!
        """

        log.d( 'discovering Acroname modules ...' )
        # see https://acroname.com/reference/_modules/brainstem/module.html#Module.discoverAndConnect
        try:
            log.debug_indent()
            for i in range(retries + 1):
                specs = brainstem.discover.findAllModules( brainstem.link.Spec.USB )
                if not specs:
                    time.sleep(1)
                else:
                    for spec in specs:
                        log.d( '...', spec )
                    break
            if not specs:
                raise self.NoneFoundError()
        finally:
            log.debug_unindent()

        return specs


    def connect(self,  reset = False, req_spec = None ):
        """
        Connect to the hub. Raises RuntimeError on failure
        :param reset: When true, the acroname will be reset as part of the connection process
        :param req_spec: Required spec to connect to.
        """
        if not self.hub:
            self.hub = brainstem.stem.USBHub3p()

        if req_spec:
            specs = [req_spec]
        else:
            specs = self.discover()

        spec = specs[0]
        result = self.hub.connectFromSpec( spec )
        if result != brainstem.result.Result.NO_ERROR:
            raise RuntimeError( "failed to connect to Acroname (result={})".format( result ))
        elif len(specs) > 1:
            log.d( 'connected to', spec )

        if reset:
            log.d("resetting Acroname...")
            result = self.hub.system.reset()
            # According to brainstem support:
            # Result error is expected, so we do not check it
            # * there is also a brainstem internal console print on Linux "error release -4"
            # Disconnection is needed after a reset command
            self.hub.disconnect()

            result = None
            for i in range(10):
                result = self.hub.connectFromSpec(spec)
                if result != brainstem.result.Result.NO_ERROR:
                    time.sleep(1)
                else:
                    log.d('reconnected')
                    return
            raise RuntimeError("failed to reconnect to Acroname (result={})".format(result))


    def find_all_hubs(self):
        """
        Yields all hub port numbers
        """
        from rspy import lsusb
        #
        # 24ff:8013 =
        #   iManufacturer           Acroname Inc.
        #   iProduct                USBHub3p-3[A]
        hubs = set( lsusb.devices_by_vendor( '24ff' ))
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


    def is_connected(self):
        return self.hub is not None and self.hub.isConnected()


    def disconnect(self):
        if self.hub:
            self.hub.disconnect()
            del self.hub
            self.hub = None


    def all_ports(self):
        """
        :return: a list of all possible ports, even if currently unoccupied or disabled
        """
        return range(8)


    def ports(self):
        """
        :return: a list of all ports currently occupied (and enabled)
        """
        occupied_ports = []
        for port in self.all_ports():
            if self._port_power(port) > 0.0:
                occupied_ports.append( port )
        return occupied_ports


    def is_port_enabled(self,  port ):
        """
        query if the input port number is enabled through Acroname
        It doesn't mean the device is enumerated as sometimes if the FW crashed we can have a situation where the port is "Disconnected" and not "OK"
        :param port: port number;
        :return: True if Acroname enabled this port, False otherwise
        """

        return self.port_state( port ) != "Disabled"


    def port_state(self,  port ):
        if port < 0 or port > 7:
            raise ValueError( "port number must be [0-7]" )
        #
        status = self.hub.usb.getPortState( port )
        #log.d("getPortState for port", port ,"return", port_state_bitmask_to_string( status.value ))
        return self.port_state_bitmask_to_string( status.value )


    def port_state_bitmask_to_string(self,  bitmask ):
        """
        https://acroname.com/reference/devices/usbhub3p/entities/usb.html#usb-port-state
        Bit   |  Port State: Result Bitwise Description
        -----------------------------------------------
        0     |  USB Vbus Enabled - Port is Disabled/Enabled by the Acroname
        1     |  USB2 Data Enabled
        2     |  Reserved
        3     |  USB3 Data Enabled
        4:10  |  Reserved
        11    |  USB2 Device Attached
        12    |  USB3 Device Attached
        13:18 |  Reserved
        19    |  USB Error Flag
        20    |  USB2 Boost Enabled
        21:22 |  Reserved
        23    |  Device Attached
        24:31 |  Reserved
        """

        if bitmask == 0: # Port is disabled by Acroname
            return "Disabled"
        if bitmask == 11: # Port is enabled but no device was detected
            return "Disconnected"
        if bitmask > 100: # Normally we hope it will cover "Device Attached" use cases (Note, can also be turn on when 'USB Error Flag' is on...but we havn't seen that )
            return "OK"
        return "Unknown Error ({})".format( bitmask )


    def enable_ports(self,  ports = None, disable_other_ports = False, sleep_on_change = 0 ):
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
            #
            if ports is None or port in ports:
                if not self.is_port_enabled( port ):
                    #log.d( "enabling port", port)
                    action_result = self.hub.usb.setPortEnable( port )
                    if action_result != brainstem.result.Result.NO_ERROR:
                        result = False
                        log.e("Failed to enable port", port)
                    else:
                        changed = True
            #
            elif disable_other_ports:
                if self.is_port_enabled( port ):
                    #log.d("disabling port", port)
                    action_result = self.hub.usb.setPortDisable( port )
                    if action_result != brainstem.result.Result.NO_ERROR:
                        result = False
                    else:
                        changed = True
        #
        if changed and sleep_on_change:
            import time
            time.sleep( sleep_on_change )
        #
        return result


    def disable_ports(self,  ports = None, sleep_on_change = 0 ):
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
                        #log.d("disabling port", port)
                        action_result = self.hub.usb.setPortDisable( port )
                        if action_result != brainstem.result.Result.NO_ERROR:
                            result = False
                            log.e("Failed to disable port", port)
                        else:
                            changed = True
        if changed and sleep_on_change:
            import time
            time.sleep( sleep_on_change )
        #
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
        import time
        time.sleep( timeout )
        #
        result = self.enable_ports( portlist ) and result
        #
        return result


    def set_ports_usb2(self,  portlist = None, timeout = 100e-3 ):
        """
        Set USB ports to USB2
        """
        if portlist is None:
            portlist = self.ports()
        #
        self.recycle_ports( portlist, timeout = timeout )
        #
        for port in portlist:
            self.hub.usb.setSuperSpeedDataEnable( port )
            self.hub.usb.setHiSpeedDataEnable( port )
            self.hub.usb.setSuperSpeedDataDisable( port )


    def set_ports_usb3(self,  portlist = None, timeout = 100e-3 ):
        """
        Set USB ports to support USB3
        """
        if portlist is None:
            portlist = self.ports()
        #
        self.recycle_ports( portlist, timeout = timeout )
        #
        for port in portlist:
            self.hub.usb.setSuperSpeedDataEnable( port )
            self.hub.usb.setHiSpeedDataEnable( port )
            self.hub.usb.setHiSpeedDataDisable( port )


    def _port_power(self,  port ):
        """
        """
        if port < 0 or port > 7:
            raise ValueError("port number can be only within 0 and 7 (inclusive)")
        #
        micro_volt = self.hub.usb.getPortVoltage( port )
        micro_curr = self.hub.usb.getPortCurrent( port )
        volt = float(micro_volt.value) / 10.0 ** 6
        amps = float(micro_curr.value) / 10.0 ** 6
        #
        return volt * amps


    def get_port_from_usb(self, first_usb_index, second_usb_index ):
        """
        Based on last two USB location index, provide the port number
        """
        acroname_port_usb_map = {(4, 4): 0,
                                 (4, 3): 1,
                                 (4, 2): 2,
                                 (4, 1): 3,
                                 (3, 4): 4,
                                 (3, 3): 5,
                                 (3, 2): 6,
                                 (3, 1): 7,
                                 }
        return acroname_port_usb_map[(first_usb_index, second_usb_index)]


if __name__ == '__main__':
    device = Acroname()
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
