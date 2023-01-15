# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

import pyrealsense2 as rs
from rspy import test
from time import sleep

#############################################################################################
# Tests
#############################################################################################

test.start("Test set empty string counter - should throw exception")
try:
    rs.aus_set("", 0)
except:
    test.finish()
else:
    test.unexpected_exception()

#############################################################################################

test.start("Test set without parameter - should throw exception")
try:
    rs.aus_set("USER_COUNTER_1")
except:
    test.finish()
else:
    test.unexpected_exception()

#############################################################################################

test.start("Test set with parameter")
rs.aus_set("USER_COUNTER_1", 5)
user_counter_1 = rs.aus_get("USER_COUNTER_1")
test.check_equal(user_counter_1, 5)
test.finish()

#############################################################################################

test.start("Test set of an already defined parameter")
rs.aus_set("USER_COUNTER_1", 8)
user_counter_1 = rs.aus_get("USER_COUNTER_1")
test.check_equal(user_counter_1, 8)
test.finish()

#############################################################################################

test.start("Test increase an already defined parameter")
rs.aus_set("USER_COUNTER_2", 0)
for n in range(10):
    rs.aus_increment("USER_COUNTER_2")
user_counter_2 = rs.aus_get("USER_COUNTER_2")
test.check_equal(user_counter_2, 10)
test.finish()

#############################################################################################

test.start("Test increase a new parameter")
for n in range(10):
    rs.aus_increment("USER_COUNTER_3")
user_counter_3 = rs.aus_get("USER_COUNTER_3")
test.check_equal(user_counter_3, 10)
test.finish()

#############################################################################################


test.start("Test timer (start, stop, get)")
rs.aus_start("TEST_TIMER")
sleep(1)
rs.aus_stop("TEST_TIMER")
sleep(1)
test_timer = rs.aus_get("TEST_TIMER")
test.check(test_timer >= 1)
test.check(test_timer <= 2)
test.finish()

#############################################################################################

test.start("Test get counters names list")
counters_list = rs.aus_get_counters_list()
expected_list = ['USER_COUNTER_1', 'USER_COUNTER_2', 'USER_COUNTER_3', 'TEST_TIMER']
test.check_equal_lists(expected_list, counters_list)
test.finish()

#############################################################################################

test.print_results_and_exit()
