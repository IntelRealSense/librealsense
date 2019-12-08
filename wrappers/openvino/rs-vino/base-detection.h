// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp>
#include <rs-vino/openvino-helpers.h>  // include all the InferenceEngine headers
#include <string>


namespace openvino_helpers
{
    /*
        Base class for any type of OpenVINO detection implementation, e.g. object_detection, age_gender_detection, etc.

        Encapsulates an ExecutableNetwork via operator->().
    */
    struct base_detection
    {
        InferenceEngine::ExecutableNetwork net;
        InferenceEngine::InferRequest::Ptr _request;
        std::string topoName;
        std::string pathToModel;
        const size_t maxBatch;
        bool isBatchDynamic;
        const bool isAsync;
        mutable bool enablingChecked;
        mutable bool _enabled;
        const bool doRawOutputMessages;

        base_detection( std::string topoName,
            const std::string &pathToModel,
            int maxBatch, bool isBatchDynamic, bool isAsync,
            bool doRawOutputMessages );

        virtual ~base_detection() = default;

        // Loads the network into the Inference Engine device
        void load_into( InferenceEngine::Core & ie, const std::string & deviceName );

        // Encapsulate the contained ExecutableNetwork
        InferenceEngine::ExecutableNetwork* operator->() { return &net; }

        // Each detector will implement its own reading of the network
        virtual InferenceEngine::CNNNetwork read_network() = 0;
            
        virtual void submit_request();
        virtual void wait();
            
        bool enabled() const;
    };
}
