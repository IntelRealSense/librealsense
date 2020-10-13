import os

n_assertions = 0
n_failed_assertions = 0
n_cases = 0
n_failed_cases = 0
case_failed = False

# Functions for asserting test cases
def p():
    global n_assertions, n_failed_assertions, n_cases, n_failed_cases
    print("n_assertion: " + str(n_assertions))
    print("n_failed_assertions: " + str(n_failed_assertions))
    print("n_test_cases: " + str(n_cases))
    print("n_failed_test_cases: " + str(n_failed_cases))
# receive an expression which is an assertion. If false the assertion failed
def require(exp):  
    global n_assertions, n_failed_assertions, case_failed
    n_assertions += 1
    if not exp:
        n_failed_assertions += 1
        case_failed = True
        print("require failed")

# This function should never be reached, it receives the number of assertion that were 
# skipped if it was reached
def require_no_reach(skipped): 
    global n_assertions, n_failed_assertions, case_failed
    n_assertions += skipped
    n_failed_assertions += skipped
    case_failed = True
    print("require_no_reach was reached")

# Functions for formating test cases
def start_case(msg):
    global n_cases, case_failed
    n_cases += 1
    case_failed = False
    print("\n\n" + msg)

def finish_case():
    global case_failed, n_failed_cases
    if case_failed:
        n_failed_cases += 1
        print("Test-Case failed")
    else:
        print("Test-Case passed")

def print_results():
    global n_assertions, n_cases, n_failed_assertions, n_failed_cases
    if n_failed_assertions:
        passed = str(n_assertions - n_failed_assertions)
        print("test cases: " + str(n_failed_cases) + " | " + str(n_cases) + " failed")
        print("assertions: " + str(n_assertions) + " | " + passed + " passed | " + str(n_failed_assertions) + " failed")
        exit(1)
    else:
        print("All tests passed (" + str(n_assertions) + " assertions in " + str(n_cases) + " test cases)")
        exit(0)
