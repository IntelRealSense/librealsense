// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "base-detection.h"


namespace openvino_helpers
{
    /*
        Detects object bounding boxes in an image.

        Can take any object detection model with a single input layer (type="Input") and
        a single output layer ("DetectionOutput").
    */
    struct object_detection : public base_detection
    {
    public:
        struct Result
        {
            int label;
            float confidence;
            cv::Rect location;
        };

    private:
        // User arguments via the ctor
        double _detection_threshold;

        // Intermediates and helpers
        std::string _input_layer_name;
        size_t _input_width;              // dimensions in the model
        size_t _input_height;
        std::string _im_info_name;        // optional
        size_t _im_info_size;
        std::string _output_layer_name;
        size_t _max_results;
        int _n_enqued_frames;
        float _width;                     // of the queued image
        float _height;

    public:
        object_detection( const std::string &pathToModel,
            double detectionThreshold,
            bool isAsync = true, 
            int maxBatch = 1, bool isBatchDynamic = false,
            bool doRawOutputMessages = false );

        InferenceEngine::CNNNetwork read_network() override;
        void submit_request() override;

        void enqueue( const cv::Mat &frame );
        std::vector< Result > fetch_results();

        float get_width() const { return _width; }
        float get_height() const { return _height; }
    };
}
