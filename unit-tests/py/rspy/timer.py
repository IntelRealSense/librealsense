# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
import time
from rspy.stopwatch import Stopwatch

# A timer counting backwards in time(vs forwards in the `stopwatch` class )
# It supply basic timer API, start, has_expired..
class Timer:

    _delta = 0
    _sw = Stopwatch()

    def __init__(self, timeout_in_sec):
        self._delta = timeout_in_sec

    # Start timer
    def start(self):
        self._sw.reset()

    # Check if timer time expired
    def has_expired(self):
        return self._sw.get_elapsed() >= self._delta

    # Force time expiration
    def set_expired(self):
        self._sw.reset(time.perf_counter() - (self._delta + 0.00001))


