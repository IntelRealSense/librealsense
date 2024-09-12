# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import running_average
import random

random.seed()  # seed random number generator


def test_around( median, plus_minus, reps = 50, sets = 10 ):
    """
    Generate random numbers around the median, keeping within 'plus_minus' of it.
    Test that, at the end, the running-average is the same as the average we calculate manually (sum(numbers)/count).
    :param median: the middle number around which we pick numbers
    :param plus_minus: how far away from the median we want to get
    :param reps: how many numbers per set of numbers
    :param sets: how many times to repeat this
    """
    test.start( f"double, {median} +/- {plus_minus}" )

    for s in range(sets):
        avg = running_average()
        tot = 0.
        log.d( f" #  value        | average      | expected" )
        for r in range(reps):
            d = random.random()  # [0,1)
            d -= 0.5  # [-0.5,0.5)
            d *= 2    # [-1,1)
            d *= plus_minus
            d += median
            avg.add( d )
            tot += d
            log.d( f"{avg.size():>3} {d:>12.2f} | {avg.get():>12.2f} | {tot / avg.size():>12.2f}" )
        test.check_equal( avg.size(), reps )
        golden = tot / avg.size()
        test.check_approx_abs( avg.get(), golden, .00001 )
        log.d()

    test.finish()


#############################################################################################
#
test_around( 5000, 100 )  # positive, small range
test_around( 100, 99 )    # positive, range large (in comparison)
test_around( 0, 100 )     # positive & negative
test_around( -100, 150 )  # more negative
test_around( -10, 5 )     # negative, small range
test_around( 100000000, 50000000 )
#
#############################################################################################
test.print_results_and_exit()
