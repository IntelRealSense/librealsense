import os, sys, subprocess, traceback

# add these 2 lines to run independently and not through run-unit-tests.py
path = "C:/Users/mmirbach/git/librealsense/build/RelWithDebInfo"
sys.path.append(path)

import pyrealsense2 as rs

n_assertions = 0
n_failed_assertions = 0
n_tests = 0
n_failed_tests = 0
test_failed = False

# If this is the first time running this script we set the wanted enviroment, However it is impossible to change the current running 
# enviroment so we rerun the script in a child process that inherits the enviroment we set
def set_env_vars(env_vars):
    if len(sys.argv) < 2:
        for env_var, val in env_vars.items():
            os.environ[env_var] = val
        sys.argv.append("rerun")
        cmd = ["py", "-3"] + sys.argv
        p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
        exit(p.returncode)

# Returns the first device that was found, if no device is found the test is aborted
def get_first_device():
    c = rs.context()
    if not c.devices.size():  # if no device is connected we don't run the AC test
        print("No device found, aborting AC test")
        exit(0)
    return c.devices[0]
# Functions for asserting test cases
# Receives an expression which is an assertion. If false the assertion failed
def require(exp):  
    global n_assertions, n_failed_assertions, test_failed
    n_assertions += 1
    if not exp:
        n_failed_assertions += 1
        test_failed = True
        print("Require failed")
        for line in traceback.format_stack():
            print(line.strip())

# Receives the resulted value and the expected value and asserts they are equal
def require_equal(result, expected):
    global n_assertions, n_failed_assertions, test_failed
    n_assertions += 1
    if not result == expected:
        n_failed_assertions += 1
        test_failed = True
        print("Result was: " + result + "\nBut we expected: " + expected)
        for line in traceback.format_stack():
            print(line.strip())

# This function should never be reached, it receives the number of assertion that were 
# skipped if it was reached
def require_no_reach(skipped):
    require(False)

# Receives 2 lists and asserts they are identical
def require_equal_lists(result, expected):
    global n_assertions, n_failed_assertions, test_failed
    n_assertions += 1
    failed = False
    if not len(result) == len(expected):
        n_failed_assertions += 1
        test_failed = True
        failed = True
        print("Require equal lists failed due to lists of different sizes:")
        print("The resulted list has " + len(result) + "elements, but the expected list has " + len(expected) + " elements")
    i = 0
    for res, exp in zip(result, expected):
        n_assertions += 1
        if not res == exp:
            n_failed_assertions += 1
            test_failed = True
            failed = True
            print("Require equal lists failed due to unequal elements:")
            print("The element of index " + str(i) + " in both lists was not equal")
    if failed:
        print("Result list:", end = ' ')
        print(result)
        print("Expected list:", end = ' ')
        print(expected)
        for line in traceback.format_stack():
            print(line.strip())

# Receives an exception and asserts its type and message is as expected
def require_exception(exception, expected_type, expected_msg = ""):
    require_equal(type(exception), expected_type)
    if expected_msg:
        require_equal(str(exception), expected_msg)

# Functions for formating test cases
def start(msg):
    global n_tests, test_failed
    n_tests += 1
    test_failed = False
    print("\n\n" + msg)

def finish():
    global test_failed, n_failed_tests
    if test_failed:
        n_failed_tests += 1
        print("Test failed")
    else:
        print("Test passed")

def print_results_and_exit():
    global n_assertions, n_tests, n_failed_assertions, n_failed_tests
    print("\n\n")
    if n_failed_assertions:
        passed = str(n_assertions - n_failed_assertions)
        print("test cases: " + str(n_failed_tests) + " | " + str(n_tests) + " failed")
        print("assertions: " + str(n_assertions) + " | " + passed + " passed | " + str(n_failed_assertions) + " failed")
        exit(1)
    else:
        print("All tests passed (" + str(n_assertions) + " assertions in " + str(n_tests) + " test cases)")
        exit(0)
