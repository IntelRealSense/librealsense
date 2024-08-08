# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

"""
USB interface library for Linux

Uses the lsusb built-in utility. Be careful to cache results or else it will
run every call!
"""

from rspy import log
import re, subprocess


def itree():
    """
    Yields all device interfaces from 'lsusb -t' in tuples:
        (bus, device, interface, ports)
    All integers, and ports is a list of port numbers.
    """
    df = subprocess.run( ['lsusb', '-t'],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True, timeout=2 )
    bus = None
    ports = []
    for i in df.stdout.split('\n'):
        if not i:
            continue
        spaces = len(i) - len(i.lstrip())
        if spaces % 4:
            raise ValueError( f'invalid number of spaces ({spaces}) in "{i}"')
        indent = int(spaces / 4)
        if indent == 0:
            match = re.search( r'Bus (\d+).', i )
            if not match:
                raise ValueError( f'expected "Bus #." in "{i}"' )
            bus = int( match.group(1) )
            ports = []
            continue
        match = re.search( r'Port (\d+): Dev (\d+), If (\d+),', i )
        if not match:
            raise ValueError( f'unexpected format in "{i}"')
            continue
        while indent <= len(ports):
            ports.pop()
        port = int( match.group(1) )
        ports.append( port )
        device = int( match.group(2) )
        interface = int( match.group(3) )
        bus_device = f'{bus}-{device}'
        yield (bus, device, interface, ports)


def tree():
    """
    Yields all devices from 'lsusb -t' in tuples:
        (device-id, ports)
    Where both are strings: '<bus>-<device>' as device-id and '<bus>-<port#>(.<port#>)*' for the
    port representation.
    """
    devices = set()
    for b,d,i,p in itree():
        device_id = f'{b}-{d}'
        if device_id not in devices:
            devices.add( device_id )
            port = f'{b}-' + '.'.join( [str(n) for n in p] )
            yield (device_id, port)


def devices_by_vendor( vid ):
    """
    """
    df = subprocess.run( ['lsusb', '-d', vid + ':'],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         universal_newlines=True, timeout=2 )
    for i in df.stdout.split('\n'):
        if i:
            match = re.search( r'^Bus (\d+) Device (\d+):', i )
            if not match:
                raise ValueError( i )
            bus = int( match.group(1) )
            device = int( match.group(2) )
            yield f'{bus}-{device}'

