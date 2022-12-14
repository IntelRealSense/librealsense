// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <deque>
#include <atomic>
#include <stddef.h>

namespace rs2
{
    // This class calculate the IR pixel reflectivity. 
    // It uses depth value history, current IR value , maximum usable depth range and noise estimation.
    // Note: This class is not thread safe
    class reflectivity 
    {
    public:
        reflectivity();
        
        // Implement algorithm requirement [RS5-8358]
        // Calculate pixel reflectivity based on current IR value and previous depth values STD
        float get_reflectivity(float raw_noise_estimation, float max_usable_range, float ir_val) const;

        // Add pixel depth value to the history queue
        void add_depth_sample(float depth_val, int x_in_image, int y_in_image);

        // Reset depth samples history (queue history will reset only if needed (if add_depth_sample was called since last reset/construction)
        void reset_history();

        // Return the samples history ratio size / capacity [0-1]
        float get_samples_ratio() const;

        // Return true if the history queue is full
        bool is_history_full() const;

        // Return the history queue capacity
        size_t history_capacity() const;

        // Return the history queue current size
        size_t history_size() const;

    private:
        // Implement a cyclic queue for depth samples
        std::deque<float> _dist_queue; 

        // Holds the current history size
        size_t _history_size;
    };
}
