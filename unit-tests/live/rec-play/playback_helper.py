# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.


import time
from rspy import log, test
from rspy.timer import Timer

'''
This class is used to wait and verify for a requested playback status arrived within a requested timeout.
It blocks the calling thread and samples the playback status using a playback status callback from LRS API
      
Usage should look like this:
    psv = PlaybackStatusVerifier( device );
    psv.wait_for_status( 5, rs.playback_status.playing )

'''
class PlaybackStatusVerifier:

    def __init__( self, dev ):
        self._current_status = None
        self._status_changes_cnt = 0
        playback_dev = dev.as_playback()
        '''
        Note: LRS set_status_changed_callback function is a 'register' and not a 'set' function
              meaning multiple set called will add a callback to the callback list
        '''
        playback_dev.set_status_changed_callback( self.__signal_on_status_change )

    def __signal_on_status_change( self, playback_status ):
        log.d('playback status callback invoked with', playback_status)
        self._status_changes_cnt += 1
        self._current_status = playback_status

    def wait_for_status( self, timeout, required_status ):
        status_changes_cnt = self._status_changes_cnt
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
            # sleep only 100 [ms],  We don't want to miss a status change if more than 1 status change occurs whithin the sleep time
            time.sleep( 0.1 )

        #If the status changes too fast let say , Stopped -> Playing --> Stopped ,
        # we want fail the test as our sleep is too long and we will not catch the required status
        test.check( status_changes_cnt + 1 == self._status_changes_cnt,
                    'Multiple status changes detected, expecting a single change, got '+ str(self._status_changes_cnt - status_changes_cnt) + ' changes' )
        test.check( required_status_detected, description = 'Check failed, Timeout on waiting for ' + str(required_status) )
