# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealdds as dds
from rspy import log
from time import sleep
import threading


def _ns_as_ms( ns ):
    return str(ns / 1000000.) + "ms"


class writer:
    """
    Just to enable simple one-line syntax:
        flexible.writer( "blah" ).write( '{"field" : 1}' )
    """

    def __init__( self, participant, topic, qos = None ):
        if type(topic) is str:
            self.handle = dds.flexible_msg.create_topic( participant, topic )
        elif type(topic) is dds.topic:
            self.handle = topic
        else:
            raise RuntimeError( "invalid 'topic' argument: " + type(topic) )
        self.n_readers = 0
        self.writer = dds.topic_writer( self.handle )
        self.writer.on_publication_matched( self._on_publication_matched )
        self.writer.run( qos or dds.topic_writer.qos() )

    def _on_publication_matched( self, writer, d_readers ):
        log.d( "on_publication_matched", d_readers )
        self.n_readers += d_readers

    def wait_for_readers( self, n_readers = 1, timeout = 3.0 ):
        while self.n_readers < n_readers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( "timed out waiting for reader" )

    def write( self, json_string ):
        msg = dds.flexible_msg( json_string )
        log.d( "writing", msg )
        msg.write_to( self.writer )

    def stop( self ):
        self.writer = self.handle = None


class reader:
    """
    Just to enable simple one-line syntax:
        msg = flexible.reader( "blah" ).read()
    """

    def __init__( self, participant, topic, qos = None ):
        if type(topic) is str:
            self.handle = dds.flexible_msg.create_topic( participant, topic )
        elif type(topic) is dds.topic:
            self.handle = topic
        else:
            raise RuntimeError( "invalid 'topic' argument: " + type(topic) )
        self.n_writers = 0
        self.data = []
        self.samples = []
        self._got_something = threading.Event()
        self.reader = dds.topic_reader( self.handle )
        self.reader.on_data_available( self._on_data_available )
        self.reader.on_subscription_matched( self._on_subscription_matched )
        self.reader.run( qos or dds.topic_reader.qos() )

    def _on_subscription_matched( self, reader, d_writers ):
        self.n_writers += d_writers
        log.d( "on_subscription_matched", reader.topic().name(), d_writers, '=', self.n_writers )

    def wait_for_writers( self, n_writers = 1, timeout = 3.0 ):
        """
        Wait until a writer/publisher has our topic open
        """
        while self.n_writers != n_writers:
            timeout -= 0.25
            if timeout > 0:
                sleep( 0.25 )
            else:
                raise RuntimeError( f"timed out waiting for {n_writers} writers" )

    def _on_data_available( self, reader ):
        topic_name = reader.topic().name()
        log.d( "on_data_available", topic_name )
        got_something = False
        while True:
            sample = dds.sample_info()
            msg = dds.flexible_msg.take_next( reader, sample )
            if not msg:
                if not got_something:
                    raise RuntimeError( "expected message not received!" )
                break
            log.d( "received", '@T+' + _ns_as_ms(sample.reception_timestamp()-sample.source_timestamp()), msg )
            self.data += [msg]
            self.samples += [sample]
            got_something = True
            self._got_something.set()

    def read_sample( self, timeout=3.0 ):
        """
        :param timeout: if empty, wait for this many seconds then raise an error
        :return: the next sample read
        """
        self.wait_for_data( timeout=timeout )
        msg = self.data.pop(0)
        if self.empty():
            self._got_something.clear()
        sample = self.samples.pop(0)
        return (msg,sample)

    def read( self, timeout=3.0 ):
        """
        :param timeout: if empty, wait for this many seconds then raise an error
        :return: the next sample read
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
