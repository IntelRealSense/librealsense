# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.


import time
from rspy import log, test
from rspy.timer import Timer

#TODO - Add description and functions comments
class PlaybackStatusVerifier:

    def __init__( self, dev ):
        self._current_status = None
        playback_dev = dev.as_playback()
        playback_dev.set_status_changed_callback( self.__signal_on_status_change )

    def __signal_on_status_change( self, playback_status ):
        log.d('playback status callback invoked with', playback_status)
        self._current_status = playback_status

    def wait_for_status( self, timeout, required_status ):
        self._current_status = None
        required_status_detected = False
        wait_for_event_timer = Timer(timeout)
        wait_for_event_timer.start()
        log.d('timeout set to', timeout, '[sec]')
        while not wait_for_event_timer.has_expired():
            if required_status == self._current_status:
                log.d('Required status "' + str(required_status) + '" detected!')
                required_status_detected = True
                break
            else:
                log.d('waiting for status "' + str(required_status) + '" event')
            # sleep only 100 [ms],  We don't want to miss a status change if more than 1 status change occurs whithin the sleep time
            time.sleep(0.1)
        test.check( required_status_detected )
