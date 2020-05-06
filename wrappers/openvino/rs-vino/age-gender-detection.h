// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "base-detection.h"


namespace openvino_helpers
{
    /*
        Detects age and gender for a give face
    */
    class age_gender_detection : public base_detection
    {
    public:
        struct Result
        {
            float age;
            float maleProb;
        };

    private:
        std::string input;
        std::string outputAge;
        std::string outputGender;
        int _n_enqued_frames;

    public:
        age_gender_detection( const std::string &pathToModel,
            bool isAsync = true,
            int maxBatch = 1, bool isBatchDynamic = false, 
            bool doRawOutputMessages = false );

        InferenceEngine::CNNNetwork read_network() override;
        void submit_request() override;

        void enqueue( const cv::Mat &face );
        Result operator[] ( int idx ) const;
    };
}
