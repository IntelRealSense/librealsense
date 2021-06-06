"""
Brainstem Acroname Hub

See documentation for brainstem here:
https://acroname.com/reference/python/index.html
"""

from rspy import log


if __name__ == '__main__':
    import os, sys, getopt
    def usage():
        ourname = os.path.basename( sys.argv[0] )
        print( 'Syntax: acroname [options]' )
        print( '        Control the acroname USB hub' )
        print( 'Options:' )
        print( '        --enable       Enable all ports' )
        print( '        --recycle      Recycle all ports' )
        sys.exit(2)
    try:
        opts,args = getopt.getopt( sys.argv[1:], '',
            longopts = [ 'help', 'recycle', 'enable' ])
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

hub = None


class NoneFoundError( RuntimeError ):
    """
    """
    def __init__( self, message = None ):
        super().__init__( self, message  or  'no Acroname module found' )


def discover():
    """
    Return all Acroname module specs in a list. Raise NoneFoundError if one is not found!
    """

    log.d( 'discovering Acroname modules ...' )
    # see https://acroname.com/reference/_modules/brainstem/module.html#Module.discoverAndConnect
    try:
        log.debug_indent()
        specs = brainstem.discover.findAllModules( brainstem.link.Spec.USB )
        if not specs:
            raise NoneFoundError()
        for spec in specs:
            log.d( '...', spec )
    finally:
        log.debug_unindent()

    return specs


def connect( spec = None ):
    """
    Connect to the hub. Raises RuntimeError on failure
    """

    global hub
    if not hub:
        hub = brainstem.stem.USBHub3p()

    if spec:
        specs = [spec]
    else:
        specs = discover()
        spec = specs[0]

    result = hub.connectFromSpec( spec )
    if result != brainstem.result.Result.NO_ERROR:
        raise RuntimeError( "failed to connect to acroname (result={})".format( result ))
    elif len(specs) > 1:
        log.d( 'connected to', spec )


def is_connected():
    try:
        connect()
        return True
    except Exception:
        return False


def disconnect():
    global hub
    hub.disconnect()
    del hub
    hub = None


def all_ports():
    """
    :return: a list of all possible ports, even if currently unoccupied or disabled
    """
    return range(8)


def ports():
    """
    :return: a list of all ports currently occupied (and enabled)
    """
    occupied_ports = []
    for port in all_ports():
        if port_power( port ) > 0.0:
            occupied_ports.append( port )
    return occupied_ports


def is_port_enabled( port ):
    return port_state( port ) == "OK"


def port_state( port ):
    if port < 0 or port > 7:
        raise ValueError( "port number must be [0-7]" )
    #
    global hub
    status = hub.usb.getPortState( port )
    #
    if status.value == 0:
        return "Disabled"
    if status.value == 11:
        return "Disconnected"
    if status.value > 100:
        return "OK"
    return "Unknown Error ({})".format( status.value )


def enable_ports( ports = None, disable_other_ports = False, sleep_on_change = 0 ):
    """
    Set enable state to provided ports
    :param ports: List of port numbers; if not provided, enable all ports
    :param disable_other_ports: if True, the ports not in the list will be disabled
    :param sleep_on_change: Number of seconds to sleep if any change is made
    :return: True if no errors found, False otherwise
    """
    global hub
    result = True
    changed = False
    for port in all_ports():
        #
        if ports is None or port in ports:
            if not is_port_enabled( port ):
                action_result = hub.usb.setPortEnable( port )
                if action_result != brainstem.result.Result.NO_ERROR:
                    result = False
                else:
                    changed = True
        #
        elif disable_other_ports:
            if is_port_enabled( port ):
                action_result = hub.usb.setPortDisable( port )
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


def disable_ports( ports ):
    """
    :param ports: List of port numbers
    :return: True if no errors found, False otherwise
    """
    global hub
    result = True
    for port in ports:
        #
        action_result = hub.usb.setPortDisable( port )
        if action_result != brainstem.result.Result.NO_ERROR:
            result = False
    #
    return result


def recycle_ports( portlist = None, timeout = 2 ):
    """
    Disable and enable a port
    :param timeout: how long to wait before re-enabling
    :return: True if everything OK, False otherwise
    """
    if portlist is None:
        portlist = ports()
    #
    result = disable_ports( portlist )
    #
    import time
    time.sleep( timeout )
    #
    result = enable_ports( portlist ) and result
    #
    return result


def set_ports_usb2( portlist = None, timeout = 100e-3 ):
    """
    Set USB ports to USB2
    """
    if portlist is None:
        portlist = ports()
    #
    recycle_ports( portlist, timeout = timeout )
    #
    global hub
    for port in portlist:
        hub.usb.setSuperSpeedDataEnable( port )
        hub.usb.setHiSpeedDataEnable( port )
        hub.usb.setSuperSpeedDataDisable( port )


def set_ports_usb3( portlist = None, timeout = 100e-3 ):
    """
    Set USB ports to support USB3
    """
    if portlist is None:
        portlist = ports()
    #
    recycle_ports( portlist, timeout = timeout )
    #
    global hub
    for port in portlist:
        hub.usb.setSuperSpeedDataEnable( port )
        hub.usb.setHiSpeedDataEnable( port )
        hub.usb.setHiSpeedDataDisable( port )


def port_power( port ):
    """
    """
    if port < 0 or port > 7:
        raise ValueError("port number can be only within 0 and 7 (inclusive)")
    #
    global hub
    micro_volt = hub.usb.getPortVoltage( port )
    micro_curr = hub.usb.getPortCurrent( port )
    volt = float(micro_volt.value) / 10.0 ** 6
    amps = float(micro_curr.value) / 10.0 ** 6
    #
    return volt * amps


def get_port_from_usb( first_usb_index, second_usb_index ):
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
    for opt,arg in opts:
        if opt in ('--enable'):
            connect()
            enable_ports()   # so ports() will return all
        elif opt in ('--recycle'):
            connect()
            enable_ports()   # so ports() will return all
            recycle_ports()
