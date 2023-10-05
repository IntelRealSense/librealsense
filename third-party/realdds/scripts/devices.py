# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

from argparse import ArgumentParser
args = ArgumentParser()
args.add_argument( '--debug', action='store_true', help='enable debug mode' )
args.add_argument( '--quiet', action='store_true', help='no output' )
def time_arg(x):
    t = int(x)
    if t <= 0:
        raise ValueError( f'--time should be a positive integer' )
    return t
args.add_argument( '--time', metavar='<seconds>', type=time_arg, default=2, help='seconds to gather devices for (default 2)' )
def domain_arg(x):
    t = int(x)
    if t <= 0 or t > 232:
        raise ValueError( f'--domain should be [0,232]' )
    return t
args.add_argument( '--domain', metavar='<0-232>', type=domain_arg, default=-1, help='DDS domain to use (default=0)' )
args = args.parse_args()


if args.quiet:
    def i( *a, **kw ):
        pass
else:
    def i( *a, **kw ):
        print( '-I-', *a, **kw )
def e( *a, **kw ):
    print( '-E-', *a, **kw )


import pyrealdds as dds
import time

dds.debug( args.debug )

settings = {}

participant = dds.participant()
participant.init( dds.load_rs_settings( settings ), args.domain )

watcher = dds.device_watcher( participant )
watcher.start()

i( 'Waiting for devices...' )
time.sleep( args.time )

for device in watcher.devices():
    di = device.device_info()
    print( di.topic_root )


