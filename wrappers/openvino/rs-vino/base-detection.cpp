// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <rs-vino/base-detection.h>
#include <easylogging++.h>


using namespace InferenceEngine;


namespace openvino_helpers
{
    base_detection::base_detection(
        std::string topoName,
        const std::string &pathToModel,
        int maxBatch, bool isBatchDynamic, bool isAsync,
        bool doRawOutputMessages
    )
        : topoName( topoName ), pathToModel( pathToModel )
        , maxBatch( maxBatch ), isBatchDynamic( isBatchDynamic ), isAsync( isAsync )
        , enablingChecked( false ), _enabled( false ), doRawOutputMessages( doRawOutputMessages )
    {
    }


    void base_detection::submit_request()
    {
        if( !enabled() || !_request )
            return;
        if( isAsync )
            _request->StartAsync();
        else
            _request->Infer();
    }


    void base_detection::wait()
    {
        if( !enabled() || !_request || !isAsync )
            return;
        _request->Wait( IInferRequest::WaitMode::RESULT_READY );
    }


    bool base_detection::enabled() const
    {
        if( !enablingChecked )
        {
            _enabled = !pathToModel.empty();
            if( !_enabled )
                LOG(INFO) << topoName + " DISABLED";
            enablingChecked = true;
        }
        return _enabled;
    }


    void base_detection::load_into( InferenceEngine::Core & ie, const std::string & deviceName )
    {
        if( !enabled() )
            return;

        std::map<std::string, std::string> config = { };
        if( isBatchDynamic )
        {
            bool isPossibleDynBatch =
                deviceName.find( "CPU" ) != std::string::npos ||
                deviceName.find( "GPU" ) != std::string::npos;
            if( isPossibleDynBatch )
                config[PluginConfigParams::KEY_DYN_BATCH_ENABLED] = PluginConfigParams::YES;
        }

        //LOG(INFO) << "Loading " << topoName << " model to the " << deviceName << " device";
        net = ie.LoadNetwork( read_network(), deviceName, config );
    }
}