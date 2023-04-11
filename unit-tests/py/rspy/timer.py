# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
import time
from rspy.stopwatch import Stopwatch

# A timer counting backwards in time(vs forwards in the `stopwatch` class )
# It supply basic timer API, start, has_expired..
class Timer:

    def __init__(self, timeout_in_sec):
        self._delta = timeout_in_sec
        self._sw = Stopwatch()

    # Start timer
    def start(self):
        self._sw.reset()

    # Get the original timeout, in seconds
    def get_timeout(self):
        return self._delta

    # Return how much time (seconds) passed since we started
    def get_elapsed(self):
        return self._sw.get_elapsed()

    # Return how much time (seconds) is left before we expire (negative if already expired)
    def time_left(self):
        return self.get_timeout() - self.get_elapsed()

    # Check if timer time expired
    def has_expired(self):
        return self.time_left() < 0

    # Force time expiration
    def set_expired(self):
        self._sw.reset(time.perf_counter() - (self._delta + 0.00001))


