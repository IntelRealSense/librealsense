import os, sys, subprocess, traceback
path = "C:/Users/mmirbach/git/librealsense/build/Debug"
sys.path.append(path)

import pyrealsense2 as rs

# If this is the first run we need to set the correct enviroment variables and rerun the script.
# Because there is no way to change the enviroment of the currently running script we must create a child process and run the script on it
# after changing the enviroment.
def set_env_vars(wanted_env):
    if len(sys.argv) < 2:
        for var,val in wanted_env.items():
            os.environ[var]=val
        sys.argv.append('rerun')
        cmd = ["py", "-3"] + sys.argv
        try:
            p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
        except Exception as e:
            print("Unexpected error occurred while running " + sys.argv[0])
            print("Error: " + str(e))
            exit(1)
        exit(p.returncode)

def get_first_device():
    c = rs.context()
    if not c.devices.size():  # If no device is connected we don't run AC tests
        print("No device found, aborting AC test")
        exit(0)
    return c.devices[0]

n_assertions = 0
n_failed_assertions = 0
n_tests = 0
n_failed_tests = 0
test_failed = False

# Functions for asserting test tests

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

# Receivs 2 arguments and asserts that they are equal
def require_equal(result, expected):
    global n_assertions, n_failed_assertions, test_failed
    n_assertions += 1
    if not result == expected:
        n_failed_assertions += 1
        test_failed = True
        print("Require failed")
        print("Expected " + str(expected) + " but received " + str(result))
        for line in traceback.format_stack():
            print(line.strip())

# This function should never be reached, it receives the number of assertion that were 
# skipped if it was reached
def require_no_reach():
    global n_assertions, n_failed_assertions, test_failed
    n_assertions += 1
    n_failed_assertions += 1
    test_failed = True
    print("require_no_reach was reached")

# Receives 2 lists, the result list and the expected result list, and asserts they are the same
def require_equal_lists(result, expected):
    require_equal(len(result), len(expected))
    for res, exp in zip(result,expected):
        require_equal(res, exp)

# Receives an Exception object and asserts its type and message are the same as expected
def require_exception(exception, expected_type, expected_msg = ""):
    require_equal(type(exception), expected_type)
    if expected_msg:
        require_equal(str(exception), expected_msg)

# Functions for formating test tests
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
        print("test tests: " + str(n_failed_tests) + " | " + str(n_tests) + " failed")
        print("assertions: " + str(n_assertions) + " | " + passed + " passed | " + str(n_failed_assertions) + " failed")
        exit(1)
    else:
        print("All tests passed (" + str(n_assertions) + " assertions in " + str(n_tests) + " test tests)")
        exit(0)
