# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.

"""
This module is for formatting and writing unit-tests in python. The general format is as follows
1. Use start() to start a test and give it, as an argument, the name of the test
2. Use whatever check functions are relevant to test the run
3. Use finish() to signal the end of the test
4. Repeat stages 1-3 as the number of tests you want to run in the file
5. Use print_results_and_exit() to print the number of tests and assertions that passed/failed in the correct format
   before exiting with 0 if all tests passed or with 1 if there was a failed test

In addition you may want to use the 'info' functions in this module to add more detailed
messages in case of a failed check
"""

import os, sys, subprocess, traceback, platform

from rspy import log

n_assertions = 0
n_failed_assertions = 0
n_tests = 0
n_failed_tests = 0
test_failed = False
test_in_progress = False
test_info = {} # Dictionary for holding additional information to print in case of a failed check.

# if --context flag was sent, the test is running under a specific context which could affect its run
context = None
if '--context' in sys.argv:
    context_index = sys.argv.index( '--context' )
    try:
        context = sys.argv.pop(context_index + 1)
    except IndexError:
        log.f( "Received context flag but no context" )
    sys.argv.pop( context_index )

# If --rslog flag was sent, enable LibRS logging (LOG_DEBUG, etc.)
try:
    sys.argv.remove( '--rslog' )
    import pyrealsense2 as rs
    rs.log_to_console( rs.log_severity.debug )
except ValueError as e:
    pass  # No --rslog passed in


def set_env_vars( env_vars ):
    """
    We want certain environment variables set when we get here. We assume they're not set.

    However, it is impossible to change the current running environment to see them. Instead, we rerun ourselves
    in a child process that inherits the environment we set.

    To do this, we depend on a specific argument in sys.argv that tells us this is the rerun (meaning child
    process). When we see it, we assume the variables are set and don't do anything else.

    For this to work well, the environment variable requirement (set_env_vars call) should appear as one of the
    first lines of the test.

    :param env_vars: A dictionary where the keys are the name of the environment variable and the values are the
        wanted values in string form (environment variables must be strings)
    """
    if sys.argv[-1] != 'rerun':
        log.d( 'environment variables needed:', env_vars )
        for env_var, val in env_vars.items():
            os.environ[env_var] = val
        cmd = [sys.executable]
        if 'site' not in sys.modules:
            #     -S     : don't imply 'import site' on initialization
            cmd += ["-S"]
        if sys.flags.verbose:
            #     -v     : verbose (trace import statements)
            cmd += ["-v"]
        cmd += sys.argv  # --debug, or any other args
        cmd += ["rerun"]
        log.d( 'running:', cmd )
        p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
        sys.exit( p.returncode )
    log.d( 'rerun detected' )
    sys.argv = sys.argv[:-1]  # Remove the rerun


def find_first_device_or_exit():
    """
    :return: the first device that was found, if no device is found the test is skipped. That way we can still run
        the unit-tests when no device is connected and not fail the tests that check a connected device
    """
    import pyrealsense2 as rs
    c = rs.context()
    if not c.devices.size():  # if no device is connected we skip the test
        log.f("No device found")
    dev = c.devices[0]
    log.d( 'found', dev )
    log.d( 'in', rs )
    return dev


def find_devices_by_product_line_or_exit( product_line ):
    """
    :param product_line: The product line of the wanted devices
    :return: A list of devices of specific product line that was found, if no device is found the test is skipped.
        That way we can still run the unit-tests when no device is connected
        and not fail the tests that check a connected device
    """
    import pyrealsense2 as rs
    c = rs.context()
    devices_list = c.query_devices(product_line)
    if devices_list.size() == 0:
        log.f( "No device of the", product_line, "product line was found" )
    log.d( 'found', devices_list.size(), product_line, 'devices:', [dev for dev in devices_list] )
    log.d( 'in', rs )
    return devices_list


def print_stack():
    """
    Function for printing the current call stack. Used when an assertion fails
    """
    log.out( 'Traceback (most recent call last):' )
    stack = traceback.format_stack()
    # Avoid stack trace into format_stack():
    #     File "C:/work/git/lrs\unit-tests\py\rspy\test.py", line 124, in check
    #       print_stack()
    #     File "C:/work/git/lrs\unit-tests\py\rspy\test.py", line 87, in print_stack
    #       stack = traceback.format_stack()
    stack = stack[:-2]
    for line in stack:
        log.out( line, end = '' )  # format_stack() adds \n


"""
The following functions are for asserting test cases:
The check family of functions tests an expression and continues the test whether the assertion succeeded or failed.
The require family are equivalent but execution is aborted if the assertion fails. In this module, the require family
is used by sending abort=True to check functions
"""


def check_failed():
    """
    Function for when a check fails
    """
    global n_failed_assertions, test_failed
    n_failed_assertions += 1
    test_failed = True
    print_info()


def abort():
    log.e( "Aborting test" )
    sys.exit( 1 )


def check( exp, description = None, abort_if_failed = False):
    """
    Basic function for asserting expressions.
    :param exp: An expression to be asserted, if false the assertion failed
    :param abort_if_failed: If True and assertion failed the test will be aborted
    :return: True if assertion passed, False otherwise
    """
    global n_assertions
    n_assertions += 1
    if not exp:
        print_stack()
        if description:
            log.out( f"    {description}" )
        else:
            log.out( f"    check failed; received {exp}" )
        check_failed()
        if abort_if_failed:
            abort()
        return False
    reset_info()
    return True


def check_equal(result, expected, abort_if_failed = False):
    """
    Used for asserting a variable has the expected value
    :param result: The actual value of a variable
    :param expected: The expected value of the variable
    :param abort_if_failed:  If True and assertion failed the test will be aborted
    :return: True if assertion passed, False otherwise
    """
    if type(expected) == list:
        log.out("check_equal should not be used for lists. Use check_equal_lists instead")
        if abort_if_failed:
            abort()
        return False
    global n_assertions
    n_assertions += 1
    if result != expected:
        print_stack()
        log.out( "    left  :", result )
        log.out( "    right :", expected )
        check_failed()
        if abort_if_failed:
            abort()
        return False
    reset_info()
    return True


def unreachable( abort_if_failed = False ):
    """
    Used to assert that a certain section of code (exp: an if block) is not reached
    :param abort_if_failed: If True and this function is reached the test will be aborted
    """
    global n_assertions
    n_assertions += 1
    print_stack()
    check_failed()
    if abort_if_failed:
        abort()


def unexpected_exception():
    """
    Used to assert that an except block is not reached. It's different from unreachable because it expects
    to be in an except block and prints the stack of the error and not the call-stack for this function
    """
    global n_assertions
    n_assertions += 1
    traceback.print_exc( file = sys.stdout )
    check_failed()


def check_equal_lists(result, expected, abort_if_failed = False):
    """
    Used to assert that 2 lists are identical. python "equality" (using ==) requires same length & elements
    but not necessarily same ordering. Here we require exactly the same, including ordering.
    :param result: The actual list
    :param expected: The expected list
    :param abort_if_failed:  If True and assertion failed the test will be aborted
    :return: True if assertion passed, False otherwise
    """
    global n_assertions
    n_assertions += 1
    failed = False
    if len(result) != len(expected):
        failed = True
        log.out("Check equal lists failed due to lists of different sizes:")
        log.out("The resulted list has", len(result), "elements, but the expected list has", len(expected), "elements")
    i = 0
    for res, exp in zip(result, expected):
        if res != exp:
            failed = True
            log.out("Check equal lists failed due to unequal elements:")
            log.out("The element of index", i, "in both lists was not equal")
        i += 1
    if failed:
        print_stack()
        log.out( "    result list  :", result )
        log.out( "    expected list:", expected )
        check_failed()
        if abort_if_failed:
            abort()
        return False
    reset_info()
    return True


def check_exception(exception, expected_type, expected_msg = None, abort_if_failed = False):
    """
    Used to assert a certain type of exception was raised, placed in the except block
    :param exception: The exception that was raised
    :param expected_type: The expected type of exception
    :param expected_msg: The expected message in the exception
    :param abort_if_failed:  If True and assertion failed the test will be aborted
    :return: True if assertion passed, False otherwise
    """
    failed = False
    if type(exception) != expected_type:
        failed = [ "    raised exception was of type", type(exception), "\n    but expected type", expected_type ]
    elif expected_msg and str(exception) != expected_msg:
        failed = [ "    exception message:", str(exception), "\n    but we expected:", expected_msg ]
    if failed:
        print_stack()
        log.out( *failed )
        check_failed()
        if abort_if_failed:
            abort()
        return False
    reset_info()
    return True


def check_frame_drops(frame, previous_frame_number, allowed_drops = 1, allow_frame_counter_reset = False):
    """
    Used for checking frame drops while streaming
    :param frame: Current frame being checked
    :param previous_frame_number: Number of the previous frame
    :param allowed_drops: Maximum number of frame drops we accept
    :return: False if dropped too many frames or frames were out of order, True otherwise
    """
    global test_in_progress
    if not test_in_progress: 
        return True
    frame_number = frame.get_frame_number()
    failed = False
    # special case for D400, because the depth sensor may reset itself
    if previous_frame_number > 0 and not (allow_frame_counter_reset and frame_number < 5):
        dropped_frames = frame_number - (previous_frame_number + 1)
        if dropped_frames > allowed_drops:
            log.out( dropped_frames, "frame(s) before", frame, "were dropped" )
            failed = True
        elif dropped_frames < 0:
            log.out( "Frames repeated or out of order. Got", frame, "after frame", previous_frame_number )
            failed = True
    if failed:
        fail() 
        return False
    reset_info()
    return True


class Information:
    """
    Class representing the information stored in test_info dictionary
    """
    def __init__(self, value, persistent = False):
        self.value = value
        self.persistent = persistent


def info( name, value, persistent = False ):
    """
    This function is used to store additional information to print in case of a failed test. This information is
    erased after the next check. The information is stored in the dictionary test_info, Keys are names (strings)
    and the items are of Information class
    If information with the given name is already stored it will be replaced
    :param name: The name of the variable
    :param value: The value this variable stores
    :param persistent: If this parameter is True, the information stored will be kept after the following check
        and will only be erased at the end of the test ( or when reset_info is called with True)
    """
    global test_info
    test_info[name] = Information(value, persistent)


def reset_info(persistent = False):
    """
    erases the stored information
    :param persistent: If this parameter is True, even the persistent information will be erased
    """
    global test_info
    if persistent:
        test_info.clear()
    else:
        new_info = test_info.copy()
        for name, information in test_info.items():
            if not information.persistent:
                new_info.pop(name)
        test_info = new_info


def print_info():
    global test_info
    if not test_info: # No information is stored
        return
    #log.out("Printing information")
    for name, information in test_info.items():
        log.out( f"    {name} : {information.value}" )
    reset_info()


def fail():
    """
    Function for manually failing a test in case you want a specific test that does not fit any check function
    """
    check_test_in_progress()
    global test_failed
    if not test_failed:
        test_failed = True


def check_test_in_progress( in_progress = True ):
    global test_in_progress
    if test_in_progress != in_progress:
        if test_in_progress:
            raise RuntimeError( "test case is already running" )
        else:
            raise RuntimeError( "no test case is running" )


def start(*test_name):
    """
    Used at the beginning of each test to reset the global variables
    :param test_name: Any number of arguments that combined give the name of this test
    """
    print_separator()
    global n_tests, test_failed, test_in_progress
    n_tests += 1
    test_failed = False
    test_in_progress = True
    reset_info( persistent = True )
    log.out( *test_name )


def finish():
    """
    Used at the end of each test to check if it passed and print the answer
    """
    check_test_in_progress()
    global test_failed, n_failed_tests, test_in_progress
    if test_failed:
        n_failed_tests += 1
        log.out("Test failed")
    else:
        log.out("Test passed")
    test_in_progress = False


def print_separator():
    """
    For use only in-between test-cases, this will separate them in some visual way so as
    to be easier to differentiate.
    """
    check_test_in_progress( False )
    global n_tests
    if n_tests:
        log.out( '\n___' )


def print_results_and_exit():
    """
    Used to print the results of the tests in the file. The format has to agree with the expected format in check_log()
    in run-unit-tests and with the C++ format using Catch
    """
    print_separator()
    global n_assertions, n_tests, n_failed_assertions, n_failed_tests
    if n_failed_tests:
        passed = n_assertions - n_failed_assertions
        log.out("test cases:", n_tests, "|" , n_failed_tests,  "failed")
        log.out("assertions:", n_assertions, "|", passed, "passed |", n_failed_assertions, "failed")
        sys.exit(1)
    log.out("All tests passed (" + str(n_assertions) + " assertions in " + str(n_tests) + " test cases)")
    sys.exit(0)
