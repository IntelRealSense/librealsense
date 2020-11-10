# This module is for formatting and writing unit-tests in python. The general format is as follows
# 1. Use start to start a test and give it, as an argument, the name of the test
# 2. Use whatever check functions are relevant to test the program run against the expected program run
# 3. Use finish to signal the end of the test
# 4. Repeat stages 1-3 as the number of tests you want to run in the file
# 5. Use print_results_and_exit to print the number of tests and assertions that passed/failed in the correct format
#    before exiting with 0 if all tests passed or with 1 if there was a failed test

import os, sys, subprocess, traceback, platform
import pyrealsense2 as rs

n_assertions = 0
n_failed_assertions = 0
n_tests = 0
n_failed_tests = 0
test_failed = False
test_in_progress = False

# If this is the first time running this script we set the wanted environment, however it is impossible to change the current running
# environment so we rerun the script in a child process that inherits the environment we set
def set_env_vars(env_vars):
    if len(sys.argv) < 2:
        for env_var, val in env_vars.items():
            os.environ[env_var] = val
        sys.argv.append("rerun")
        if platform.system() == 'Linux' and "microsoft" not in platform.uname()[3].lower():
            cmd = ["python3"] + sys.argv
        else:
            cmd = ["py", "-3"] + sys.argv
        p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
        exit(p.returncode)

# Returns the first device that was found, if no device is found the test is skipped. That way we can still run
# the unit-tests when no device is connected and not fail the tests that check a connected device
def find_first_device_or_exit():
    c = rs.context()
    if not c.devices.size():  # if no device is connected we skip the test
        print("No device found, skipping test")
        exit(0)
    return c.devices[0]

# Function for printing the current call stack. Used when an assertion fails
def print_stack():
    for line in traceback.format_stack():
        print(line)

# Functions for asserting test cases:
# The check family of functions tests an expression and continues the test whether the assertion succeeded or failed.
# The require family are equivalent but execution is aborted if the assertion fails.

# Function for when a check fails
def check_failed():
    global n_failed_assertions, test_failed
    n_failed_assertions += 1
    test_failed = True

def abort_test():
    print("Abort was specified in a failed check. Aborting test")
    exit(1)

# Receive an expression which is an assertion. If false the assertion failed.
def check(exp, abort = False):
    global n_assertions
    n_assertions += 1
    if not exp:
        print("Check failed, received", end = " ")
        print(exp)
        check_failed()
        print_stack()
        if abort:
            abort_test()


# Receives the resulted value and the expected value and asserts they are equal
def check_equal(result, expected, abort = False):
    check(type(expected) != list)
    global n_assertions
    n_assertions += 1
    if not result == expected:
        print("Result was: " + result + "\nBut we expected: " + expected)
        check_failed()
        print_stack()
        if abort:
            abort_test()

# This function should never be reached
def check_not_reached( abort = False ):
    check(False, abort)

# Receives 2 lists and asserts they are identical. python "equality" (using ==) requires same length & elements
# but not necessarily same ordering. Here we require exactly the same, including ordering.
def check_equal_lists(result, expected, abort = False):
    global n_assertions
    n_assertions += 1
    failed = False
    if not len(result) == len(expected):
        failed = True
        print("Check equal lists failed due to lists of different sizes:")
        print("The resulted list has " + str(len(result)) + " elements, but the expected list has " + str(len(expected)) + " elements")
    i = 0
    for res, exp in zip(result, expected):
        if not res == exp:
            failed = True
            print("Check equal lists failed due to unequal elements:")
            print("The element of index " + str(i) + " in both lists was not equal")
        i += 1
    if failed:
        print("Result list:", end = ' ')
        print(result)
        print("Expected list:", end = ' ')
        print(expected)
        check_failed()
        print_stack()
        if abort:
            abort_test()

# Receives an exception and asserts its type and message is as expected. This function is called with a caught exception,
def check_exception(exception, expected_type, expected_msg = None, abort = False):
    check_equal(type(exception), expected_type, abort)
    if expected_msg:
        check_equal(str(exception), expected_msg, abort)

# Functions for formatting test cases
def start(test_name = "Test started"):
    global n_tests, test_failed, test_in_progress
    if test_in_progress:
        print("Tried to start test before previous test finished. Aborting test")
        exit(1)
    n_tests += 1
    test_failed = False
    test_in_progress = True
    print(test_name)

def finish():
    global test_failed, n_failed_tests, test_in_progress
    if not test_in_progress:
        print("Tried to finish a test without starting one")
        return
    if test_failed:
        n_failed_tests += 1
        print("Test failed")
    else:
        print("Test passed")
    print("\n")
    test_in_progress = False

# The format has to agree with the expected format in check_log() in run-unit-tests and with the C++ format using Catch
def print_results_and_exit():
    global n_assertions, n_tests, n_failed_assertions, n_failed_tests
    if n_failed_assertions:
        passed = str(n_assertions - n_failed_assertions)
        print("test cases: " + str(n_tests) + " | " + str(n_failed_tests) + " failed")
        print("assertions: " + str(n_assertions) + " | " + passed + " passed | " + str(n_failed_assertions) + " failed")
        exit(1)
    print("All tests passed (" + str(n_assertions) + " assertions in " + str(n_tests) + " test cases)")
    exit(0)
