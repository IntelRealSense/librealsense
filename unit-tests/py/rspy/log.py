# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

import sys


# Set up the default output system; if not a terminal, disable colors!
def _stream_has_color( stream ):
    if not hasattr(stream, "isatty"):
        return False
    if not stream.isatty():
        return False # auto color only on TTYs
    try:
        import curses
        curses.setupterm()
        return curses.tigetnum( "colors" ) > 2
    except:
        # guess false in case of error
        return False

if _stream_has_color( sys.stdout ):
    red = '\033[91m'
    gray = '\033[90m'
    reset = '\033[0m'
    cr = '\033[G'
    clear_eos = '\033[J'
    clear_eol = '\033[K'
    _progress = ''
    def out(*args):
        print( *args, end = clear_eol + '\n' )
        global _progress
        if len(_progress):
            progress( *_progress )
    def progress(*args):
        print( *args, end = clear_eol + '\r' )
        global _progress
        _progress = args
else:
    red = gray = reset = cr = clear_eos = ''
    def out(*args):
        print( *args )
    def progress(*args):
        print( *args )


def quiet_on():
    print( "QUIET ON" )
    global out
    def out(*args):
        pass


def v(*args):
    pass
def verbose_on():
    global v
    def v(*args):
        global gray, reset
        out( gray + '-V-', *args, reset )


def d(*args):
    pass
def debug_on():
    global d
    def d(*args):
        global gray, reset
        out( gray + '-D-', *args, reset )


def i(*args):
    out( '-I-', *args)


# We track the number of errors
_n_errors = 0
def e(*args):
    global red, reset
    out( red + '-E-' + reset, *args )
    global _n_errors
    _n_errors = _n_errors + 1

def n_errors():
    global _n_errors
    return _n_errors

def reset_errors():
    global _n_errors
    _n_errors = 0

