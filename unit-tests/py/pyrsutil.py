# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
import time

# Timer that counts forward in time (vs backwards in the 'timer' class)
# It supply a basic stopwatch API, reset, get elapsed time..
class stopwatch:

    _start = 0

    def __init__(self):
        self._start = time.perf_counter()

    # Reset the stopwatch time
    def reset(self):
        self._start = time.perf_counter()

    # Get elapsed since timer creation
    def get_elapsed(self):
        return time.perf_counter() - self._start

    # Get stopwatch start time
    def get_start(self):
        return self._start

# A timer counting backwards in time(vs forwards in the `stopwatch` class )
# It supply basic timer API, start, has_expired..
class timer:

    _delta = 0
    _sw = stopwatch()

    def __init__(self, timeout):
        self._delta = timeout

    # Start timer
    def start(self):
        self._sw.reset()

    # Check if timer time expired
    def has_expired(self):
        return self._sw.get_start() + self._delta <= time.perf_counter()

    # Force time expiration
    def set_expired(self):
        self._sw.reset(time.perf_counter() - (self._delta + 0.00001))


