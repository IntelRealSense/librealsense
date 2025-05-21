# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

from rspy import log
import sys, signal

signal_handler = None

def register_signal_handlers(on_signal=None):
    def handle_abort(signum, _):
        global signal_handler
        log.w("got signal", signum, "aborting... ")
        signal_handler()
        sys.exit(1)

    global signal_handler
    if signal_handler is None:
        signal_handler = on_signal
    signal.signal(signal.SIGTERM, handle_abort) # for when aborting via Jenkins
    signal.signal(signal.SIGINT, handle_abort) # for Ctrl+C
