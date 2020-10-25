// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <numeric>
#include "reflectivity.h"
#include "types.h"

using namespace rs2;


static const int STD_PERIOD = 100;
static const int NINETY_FIVE_PERCENT_OF_STD_PERIOD = static_cast<int>(0.95 * STD_PERIOD);
static const int MAX_RANGE_IN_UNIT = 65536.0f;
static const float LONG_THERMAL = 74.5f;
static const float INDOOR_MAX_RANGE = 9.0f;

//TODO try to read from LRS
static const float FOV_H = 0.610865f;
static const float FOV_V = 0.479966f;

static const int VGA_HALF_WIDTH = 320;
static const int VGA_HALF_HEIGHT = 240;

static bool is_close_to_zero(float x) { return (std::abs(x) < std::numeric_limits<float>::epsilon()); }


reflectivity::reflectivity() : _is_empty(false)
{
    _dist_queue.assign(STD_PERIOD, 0); // Allocate size for all samples in advance to minimize runtime.
}

//Reflectivity analysis is based on STD and normalized IR.Final reflectivity values are weighted average between them
float reflectivity::get_reflectivity(float raw_noise_estimation,float max_usable_range, float ir_val) const
{
    //Calculating STD over time(temporal noise), when there are more than 5 % invalid return high value
    std::vector<float> _filt_dist_arr(STD_PERIOD);

    int count_valid = 0;

    // Count non zero (or close to zero) elements in history
    std::copy_if(_dist_queue.begin(), _dist_queue.end(), _filt_dist_arr.begin(),
        [&count_valid]( float elem ) {
        if (!is_close_to_zero(elem))
        {
            ++count_valid;
            return true;
        }
        return false;
    } );

    if (0 == count_valid)
        throw librealsense::invalid_value_exception("reflectivity N/A, not enough data was collected");

    _filt_dist_arr.resize(count_valid); // resize to actual number of elements pushed

    float sum = std::accumulate( _filt_dist_arr.begin(), _filt_dist_arr.end(), 0.0f );
    float mean = sum / _filt_dist_arr.size();

    float standard_deviation = 9999.0f;

    if( count_valid >= NINETY_FIVE_PERCENT_OF_STD_PERIOD )
    {
        float variance = 0.0f;
        for( auto val : _filt_dist_arr )
            variance += pow( val - mean, 2.0f );

        variance = variance / _filt_dist_arr.size();
        standard_deviation = sqrt( variance );
    }

    //Range is calculated based on position in map(assuming 0 tilt) Distance is just based on plane distance
    auto range = mean / 1000.0f;

    //Normalized IR instead of IR is used to be robust to high ambient, RSS is assumed between echo to noise
    auto nest = raw_noise_estimation / 16.0f;
    auto normalized_ir = 0.0f;
    if (ir_val > nest) // # NIR - IRVal is the IR value of the current pixel the mouse is peeking to / Nest is the normalized NEST
        normalized_ir = pow(pow(ir_val, 2.0f) - pow(nest, 2.0f), 0.5f);

    auto ref_from_ir = 0.0f;
    auto ref_from_std = 0.0f;
    auto i_ref = 0.0f;
    auto s_ref = 0.0f;
    auto i_dist_85 = 0.0f;
    auto s_dist_85 = 0.0f;
    auto ref = 0.0f;


    if (nest <= LONG_THERMAL * 2.0f)
    {
        //We can hold reflectivity information from IR
        if (normalized_ir < 70.0f)
            ref_from_ir = 0.5f; //Low confidence as IR values are low
        else
            ref_from_ir = 1.0f; //High confidence as IR values hold good SNR

        //Analyzing reflectivity based on 85 % reflectivity data
        i_dist_85 = 6.75f * std::exp( -0.012f * normalized_ir );

        i_dist_85 = i_dist_85 * max_usable_range / INDOOR_MAX_RANGE;

        if (0.0f < i_dist_85) // protect devision by zero
        {
            i_ref = 0.85f * pow((range / i_dist_85), 2);

            if (i_ref > 0.95f)
            {
                ref_from_ir = 0.75f;
                i_ref = 0.95f;
            }
        }
        else
            ref_from_ir = 0.0f;
    }

    // STD based analysis
    // STD values are at the edges, hard to get data
    if (standard_deviation >= 1.5f || standard_deviation <= 10.f)
    {
        if (standard_deviation > 2.5f)
            ref_from_std = 0.5f;  // Low confidence data due to low STD values
        else
            ref_from_std = 1.0f;  // High confidence data

        // STD based analysis based on 85% reflectivity data
        if (range > 0.0f)
        {
            s_dist_85 = (2.25f * (log(standard_deviation)) + 1.56f) * max_usable_range / INDOOR_MAX_RANGE;
            if (0 < s_dist_85) // protect devision by zero
            {
                s_ref = 0.85f * pow((range / s_dist_85), 2.0f);
                if (s_ref > 0.95f)
                {
                    ref_from_std = ref_from_std * 0.75f;
                    s_ref = 0.95f;
                }
            }
            else
                ref_from_std = 0.0f;
        }
        else
        {
            ref_from_std = 0.0f;
        }

    }
    // Calculating Final reflectivity
    if(is_close_to_zero( ref_from_ir + ref_from_std ) )
        throw librealsense::invalid_value_exception( "reflectivity N/A" );


    ref = ( i_ref * ref_from_ir + s_ref * ref_from_std ) / (ref_from_ir + ref_from_std );

    return ref;
}

void reflectivity::add_depth_sample(float depth_val, int x_in_image, int y_in_image)
{
    auto dist_z = round(depth_val * 16000.f / MAX_RANGE_IN_UNIT); // convert to mm units
    float x_ang = FOV_H * abs(VGA_HALF_WIDTH - x_in_image) / VGA_HALF_WIDTH;
    float y_ang = FOV_V * abs(VGA_HALF_HEIGHT - y_in_image) / VGA_HALF_HEIGHT;
    auto dist_r = dist_z * sqrt(1.0f + (pow(2.0f * tan(x_ang), 2.0f) + pow(2 * tan(y_ang), 2.0f)) / 4.0f);

    if (_dist_queue.size() >= STD_PERIOD) // Keep queue as STD_PERIOD size queue
        _dist_queue.pop_front();

    _dist_queue.push_back(dist_r);
    _is_empty = false;
}

void rs2::reflectivity::reset_history()
{
    if (!_is_empty)
    {
        _dist_queue.assign(STD_PERIOD, 0);
        _is_empty = true;
    }
}
