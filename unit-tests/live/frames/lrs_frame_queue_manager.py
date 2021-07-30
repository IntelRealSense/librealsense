# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.
# This library is part of validation testing wrapper

import collections
import csv
import logging
import os
import threading
import time
from queue import Queue, Full
from datetime import datetime

import pyrealsense2 as rs


class ModifiedLogger(logging.LoggerAdapter):
    def process(self, msg, kwargs):
        return "{serial}: {msg}".format(serial=self.extra['serial'], msg=msg), kwargs


class LRSFrameQueueManager:
    """Manage a queue structure for drop-safe and high-performance frame handling using pyrealsense.

    The queue structure consist of 2 queues: lrs frame_queue and python threading queue.
    The lrs frame_queue is filled with frames arriving from lrs while the threading queue get frames from it
    and send it for post-process.

    Each frame which is inserted to the lrs.frame_queue is marked with keep=True
    to prevent frame drops due to slow frame processing.
    Frames from the lrs frame_queue are sent for post-process using producer-consumer threads;
    the producer thread get frames from the lrs frame_queue and put it in the threading queue, while
    the consumer thread get frames from the threading queue and sent it for post-process.

    Attributes:
        lrs_queue: pyrealsesne.frame_queue through it frames arrive from lrs

    Args:
        callback (function): Post-process callback to be called on each frame in the post process queue.
        max_queue_size (int): Max size of a queue in the frame queues structure.
        statistics (bool): Whether to collect performance statistics or not.
        If collected, will be saved in a CSV file under '~/queue_<ts>.csv
        serial_number (str): The serial number of the camera of which the queues belongs to.
    """

    class Event(object):
        TERMINATE_CONSUMER = 'terminate_consumer'
        START_COLLECTING = 'start_collecting'
        STOP_COLLECTING = 'stop_collecting'

    def __init__(self, callback=None, max_queue_size=100000, statistics=False, serial_number=None):
        self._id = serial_number
        self._max_queue_size = max_queue_size

        self._logger = ModifiedLogger(logging.getLogger('test'), {'serial': self._id}, )
        self._process = callback

        self.lrs_queue = rs.frame_queue(capacity=self._max_queue_size, keep_frames=True)

        self._post_process_queue = Queue(maxsize=self._max_queue_size)
        self._producer_thread = None
        self._consumer_thread = None
        self._block_queue_event = threading.Event()
        self.block()  # ignoring frames until self.start is called
        self._terminate_producer_event = threading.Event()

        self.statistics = statistics

        self._producing_times = collections.deque()
        self._producer_queue_sizes = collections.deque()
        self._consuming_times = collections.deque()
        self._consumer_queue_sizes = collections.deque()
        self._memory_samples = collections.deque()

    def start(self):
        """Initialize the queue structure and starting the producer-consumer threads

        first, unblocking the threading queue and clear all thread's events and then
        initialize the producer-consumer threads.
        """

        self.unblock()
        self._terminate_producer_event.clear()

        if self.lrs_queue is None:
            self.lrs_queue = rs.frame_queue(capacity=self._max_queue_size, keep_frames=True)
        if self._post_process_queue is None:
            self._post_process_queue = Queue(maxsize=self._max_queue_size)

        # in case self.start is being called multiple times in a row and self.stop is not being called between.
        if self._producer_thread is None:
            self._logger.info("initializing a producer thread")
            self._producer_thread = threading.Thread(target=self._produce_frames, name="producer_thread(LRSFrameQueueManager)")
            self._producer_thread.setDaemon(True)
            self._logger.info("starting the producer thread")
            self._producer_thread.start()

        # # in case self.start is being called multiple times in a row and self.stop is not being called between.
        if self._consumer_thread is None:
            self._logger.info("initializing a consumer thread")
            self._consumer_thread = threading.Thread(target=self._consume_frames, name="consumer_thread(LRSFrameQueueManager)")
            self._consumer_thread.setDaemon(True)
            self._logger.info("starting the consumer thread")
            self._consumer_thread.start()

    def stop(self, timeout=3 * 60):
        """Kill the producer-consumer threads and deconstruct the queues.

        Args:
            timeout (float): max time to kill consumer/producer thread.
        """
        # saving the post-process queue state so it will be returned to its initial state after it will be blocked
        leave_unblocked = not self._block_queue_event.is_set()

        self.block()

        self._logger.info("waiting for frames queue to be empty and frames consuming thread to be terminated")
        self._logger.info("inserting 'termination' object to frame queue")
        self._post_process_queue.put((LRSFrameQueueManager.Event.TERMINATE_CONSUMER, time.time()))
        self._consumer_thread.join(timeout=timeout)
        if self._consumer_thread.is_alive():
            raise RuntimeError("frame queue hasn't been empty and frame consuming thread hans't been terminated within {} seconds".format(timeout))
        self._logger.info("frames queue is empty and consuming thread was terminated")
        self._consumer_thread = None

        self._logger.info("killing producer thread")
        self._terminate_producer_event.set()
        self._producer_thread.join(timeout=timeout)
        if self._producer_thread.is_alive():
            raise RuntimeError("frame producing thread han't been terminated within {} seconds".format(timeout))
        self._logger.info("producing thread was terminated")
        self._producer_thread = None

        del self.lrs_queue
        self.lrs_queue = None

        del self._post_process_queue
        self._post_process_queue = None

        if leave_unblocked:
            self.unblock()

    def block(self):
        """Stop the producer thread from putting the frames from lrs frame_queue in the threading queue.
        """
        self._logger.info("blocking the post-process queue")
        self._block_queue_event.set()

    def unblock(self):
        """set the producer thread to put the frames from lrs frame_queue in the threading queue.
        """
        self._logger.info("unblocking the post-process queue")
        self._block_queue_event.clear()

    def join(self):
        """Block until the threading queue is empty.
        """
        self._logger.info("waiting fot the post-process queue to be empty")
        if not self._post_process_queue:
            # in case LRSFrameQueueManager.join() was called after LRSFrameQueueManager.stop()
            return
        self._post_process_queue.join()

    def register_callback(self, callback):
        """Register a post-process callback to be called on each frame in the post process queue.

        Args:
            callback: post-process callback to be called on each frame in the post process queue.
        """
        self._logger.info("registering post-process callback")
        self._process = callback

    def _produce_frames(self, timeout=1):
        while True:
            start = time.time()
            if self._terminate_producer_event.is_set():
                self._logger.debug("producer thread have been set to be terminated, returning")
                return
            # self._logger.debug("waiting for a frame in lrs_queue")
            try:
                lrs_frame = self.lrs_queue.wait_for_frame(timeout_ms=timeout * 1000)
                frame_ts = time.time() * 1000.0  # [milliseconds]
            except Exception as ex:
                self._logger.error(ex)
                continue
            # self._logger.debug("got a frame from lrs_queue")
            if self._block_queue_event.is_set():
                self._logger.debug("queue is blocked, dropped frame #{} of stream {}".format(lrs_frame.get_frame_number(), lrs_frame.get_profile().stream_type()))
                continue
            # self._logger.debug("putting the frame in the queue")
            try:
                self._post_process_queue.put((lrs_frame, frame_ts), block=True, timeout=timeout)
            except Full:
                self._logger.error("frame queue is full for more than {} seconds, dropped frame #{}".format(timeout, lrs_frame.get_frame_number()))
                continue
            produce_time = (time.time() - start) * 1000.0
            queue_size = self._post_process_queue.qsize()
            if self.statistics:
                self._producing_times.append(produce_time)
                self._producer_queue_sizes.append(queue_size)
            self._logger.debug("added frame to the queue within: {} ms, queue size: {}".format(produce_time, queue_size))

    def _consume_frames(self):
        while True:
            # self._logger.debug("getting a frame from the queue, queue size: {}".format(self._post_process_queue.qsize()))
            element, ts = self._post_process_queue.get(block=True)
            start = time.time()
            if element == LRSFrameQueueManager.Event.TERMINATE_CONSUMER:
                self._logger.debug("consuming thread has been set to be terminated, returning")
                return
            lrs_frame = element
            # self._logger.debug("working on the frame")
            try:
                if self._process:
                    self._process(lrs_frame, ts)
                else:
                    self._logger.debug("no post-process callback is configured, dropping frame #{} of stream {}".format(lrs_frame.get_frame_number(), lrs_frame.get_profile().stream_type()))
                    continue
            except Exception as ex:
                self._logger.exception(ex)
            del lrs_frame
            consume_time = (time.time() - start) * 1000.0
            queue_size = self._post_process_queue.qsize()
            # self._logger.debug("marking the frame as a done task")
            self._post_process_queue.task_done()
            self._logger.debug("consumed a frame from the queue within {} ms, queue size: {}".format(consume_time, queue_size))
            if self.statistics:
                self._consumer_queue_sizes.append(queue_size)
                self._consuming_times.append(consume_time)

    def to_csv(self):
        """Save internal performance statistics gathered by the producer and consumer threads in a CSV file under '~/queue_<ts>.csv'
        """
        if self.statistics:
            with open(os.path.join(os.path.expanduser('~'), 'queue_{}.csv'.format(datetime.now().strftime("%d-%m-%Y-%H-%M-%S.%f"))), 'wb') as csv_file:
                fieldnames = ['producing_time', 'producer_queue_size', 'consuming_time', 'consumer_queue_size', 'memory']
                writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
                writer.writeheader()
                for producing_time, producer_queue_size, consuming_time, consumer_queue_size, memory_sample in zip(self._producing_times, self._producer_queue_sizes, self._consuming_times, self._consumer_queue_sizes, self._memory_samples):
                    writer.writerow({'producing_time': producing_time, 'producer_queue_size': producer_queue_size, 'consuming_time': consuming_time, 'consumer_queue_size': consumer_queue_size, 'memory': memory_sample})
