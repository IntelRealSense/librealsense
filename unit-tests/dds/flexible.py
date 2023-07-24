# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log
from time import sleep
import threading
from pyrealdds import timestr, no_suffix, rel, abs


class writer:
    """
    Just to enable simple one-line syntax:
        flexible.writer( participant, "blah" ).write( '{"field" : 1}' )
    """

    def __init__( self, participant, topic, qos = None ):
        if type(topic) is str:
            self.handle = dds.message.flexible.create_topic( participant, topic )
        elif type(topic) is dds.topic:
            self.handle = topic
        else:
            raise RuntimeError( "invalid 'topic' argument: " + type(topic) )
        self.n_readers = 0
        self.name = self.handle.name()
        self.writer = dds.topic_writer( self.handle )
        self.writer.on_publication_matched( self._on_publication_matched )
        self.writer.run( qos or dds.topic_writer.qos() )

    def _on_publication_matched( self, writer, d_readers ):
        n_readers = self.n_readers + d_readers
        log.d( f'{self.name}.on_publication_matched {d_readers:+} -> {n_readers}' )
        self.n_readers = n_readers

    def wait_for_readers( self, n_readers = 1, timeout = 3.0 ):
        while self.n_readers < n_readers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( f'timed out waiting for {n_readers} readers' )

    def write( self, json_string ):
        msg = dds.message.flexible( json_string )
        now = dds.now()
        msgs = str(msg)
        msg.write_to( self.writer )
        log.d( f'{self.name}.write {msgs} @{timestr(now,no_suffix)}{timestr(dds.now(),now)}' )

    def stop( self ):
        self.writer = self.handle = None


class reader:
    """
    Just to enable simple one-line syntax:
        msg = flexible.reader( participant, "blah" ).read()
    """

    def __init__( self, participant, topic, qos = None ):
        if type(topic) is str:
            self.handle = dds.message.flexible.create_topic( participant, topic )
        elif type(topic) is dds.topic:
            self.handle = topic
        else:
            raise RuntimeError( "invalid 'topic' argument: " + type(topic) )
        self.n_writers = 0
        self.name = self.handle.name()
        self.data = []
        self.samples = []
        self._got_something = threading.Event()
        self.reader = dds.topic_reader( self.handle )
        self.reader.on_data_available( self._on_data_available )
        self.reader.on_subscription_matched( self._on_subscription_matched )
        self.reader.run( qos or dds.topic_reader.qos() )

    def _on_subscription_matched( self, reader, d_writers ):
        n_writers = self.n_writers + d_writers
        log.d( f'{self.name}.on_subscription_matched {d_writers:+} -> {n_writers}' )
        self.n_writers = n_writers

    def wait_for_writers( self, n_writers = 1, timeout = 3.0 ):
        """
        Wait until a writer/publisher has our topic open
        """
        while self.n_writers != n_writers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( f'{self.name} timed out waiting for {n_writers} writers' )

    def _on_data_available( self, reader ):
        now = dds.now()
        #log.d( f'{topic_name}.on_data_available @{now}' )
        got_something = False
        while True:
            sample = dds.message.sample_info()
            msg = dds.message.flexible.take_next( reader, sample )
            if not msg:
                if not got_something:
                    raise RuntimeError( "expected message not received!" )
                break
            received = sample.reception_timestamp()
            log.d( f'{self.name}.on_data_available @{timestr(received,no_suffix)}{timestr(now,received,no_suffix)}{timestr(dds.now(),now)} {msg}' )
            self.data += [msg]
            self.samples += [sample]
            got_something = True
            self._got_something.set()

    def read_sample( self, timeout=3.0 ):
        """
        :param timeout: if empty, wait for this many seconds then raise an error
        :return: a tuple of the next flexible message, and the sample read
        """
        self.wait_for_data( timeout=timeout )
        msg = self.data.pop(0)
        if self.empty():
            self._got_something.clear()
        sample = self.samples.pop(0)
        log.d( f'{self.name}.read_sample @{timestr(dds.now(),sample.reception_timestamp())}' )
        return (msg,sample)

    def read( self, timeout=3.0 ):
        """
        :param timeout: if empty, wait for this many seconds then raise an error
        :return: the next flexible message read
        """
        msg, sample = self.read_sample( timeout=timeout )
        return msg

    def wait_for_data( self, timeout = 3.0 ):
        """
        Wait until data is available or timeout seconds. If still none, raises an error.
        """
        if not self._got_something.wait( timeout ):
            raise RuntimeError( "timed out waiting for data" )

    def empty( self ):
        """
        :return: True if no data is available
        """
        return not self.data
