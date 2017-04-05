## License: Apache 2.0. See LICENSE file in root directory.
## Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#################################################
## pybackend example #1 - Accessing depth data ##
#################################################

# First import the library
import pybackend2

try:
    # Create a backend object.
    backend = pybackend2.create_backend()

    # Query all connected uvc devices
    infos = backend.query_uvc_devices()
    print("There are %d connected UVC devices" % len(devices))
    if len(devices) is 0: exit(1)

    # Set up the first device
    dev = backend.create_uvc_device(infos[0])

    # Configure to run at VGA resolution at 30 frames per second
    dev.probe_and_commit(

    print dev
    pass
except:
    pass