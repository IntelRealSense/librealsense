# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

from rspy import log, test
from pyrsutils import running_average_i as running_average
import random

random.seed()  # seed random number generator


def signed( n ):
    s = str(n)
    if n >= 0:
        s = '+' + s
    return s


def test_set( median, plus_minus, reps = 50 ):
    avg = running_average()
    tot = 0
    log.d( f" #  value        | average                                                               | expected" )
    for r in range(reps):
        d = random.random()  # [0,1)
        d -= 0.5  # [-0.5,0.5)
        d *= 2    # [-1,1)
        d *= plus_minus
        d += median
        d = int(d)
        prev = avg.get()
        lo = avg.leftover()
        avg.add( d )
        tot += d
        rounding = int(avg.size() / 2)
        if prev + lo < 0:
            rounding = -rounding
        log.d( f"{avg.size():>3} {d:>12} | {str(prev)+'+('+signed(d-prev)+signed(lo)+signed(rounding)+')/'+str(avg.size()):>35}= {str(avg.get())+signed(avg.leftover()):>15}= {avg.get_double():>15.2f} | {tot / avg.size():>12.2f}" )
    test.check_equal( avg.size(), reps )
    golden = tot / avg.size()
    test.check_approx_abs( avg.get(), golden, 1. )
    # We have higher expectations! We should get a pretty nice match:
    test.check_approx_abs( float( avg ), golden, .001 )
    log.d()


def test_around( median, plus_minus, reps = 50, sets = 10 ):
    """
    Generate random numbers around the median, keeping within 'plus_minus' of it.
    Test that, at the end, the running-average is the same as the average we calculate manually (sum(numbers)/count).
    :param median: the middle number around which we pick numbers
    :param plus_minus: how far away from the median we want to get
    :param reps: how many numbers per set of numbers
    :param sets: how many times to repeat this
    """
    test.start( f"int, {median} +/- {plus_minus}" )

    for s in range(sets):
        # in case we want to reproduce, you can take the output, a tuple:
        #     random.setstate( (3, (...), None) )
        #     test_set(
        log.d( f'random.setstate( {random.getstate()} )  &&  test_set( {median}, {plus_minus}, {reps} )' )
        test_set( median, plus_minus, reps )

    test.finish()


#############################################################################################
#
# These use random numbers, so the output will be different each time...
# See the above debug output if something needs to be reproduced.
#
test_around( 5000, 100 )  # positive, small range
test_around( 100, 99 )    # positive, range large (in comparison)
test_around( 0, 100 )     # positive & negative
test_around( -100, 150 )  # more negative
test_around( -10, 5 )     # negative, small range
test_around( 100000000, 50000000 )  # large numbers
test_around( 0, 50000000 )  # with negatives
#
#############################################################################################
test.print_results_and_exit()
