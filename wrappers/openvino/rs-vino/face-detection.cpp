// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.


#include <rs-vino/face-detection.h>
#include <rs-vino/openvino-helpers.h>
#include <easylogging++.h>


using namespace InferenceEngine;


const size_t DETECTED_OBJECT_SIZE = 7;   // the size of each detected object


namespace openvino_helpers
{
    face_detection::face_detection(
        const std::string &pathToModel,
        double detectionThreshold,
        bool isAsync,
        int maxBatch, bool isBatchDynamic,
        bool doRawOutputMessages
    )
        : base_detection( "face detection", pathToModel, maxBatch, isBatchDynamic, isAsync, doRawOutputMessages )
        , _detection_threshold( detectionThreshold )
        , _max_results( 0 )
        , _n_enqued_frames( 0 ), _width( 0 ), _height( 0 )
    {
    }


    void face_detection::submit_request()
    {
        if( !_n_enqued_frames )
            return;
        _n_enqued_frames = 0;
        base_detection::submit_request();
    }


    void face_detection::enqueue( const cv::Mat & frame )
    {
        if( !enabled() )
            return;

        if( !_request )
            _request = net.CreateInferRequestPtr();

        _width = static_cast<float>(frame.cols);
        _height = static_cast<float>(frame.rows);

        Blob::Ptr  inputBlob = _request->GetBlob( _input_layer_name );
        matU8ToBlob<uint8_t>( frame, inputBlob );

        _n_enqued_frames = 1;
    }


    CNNNetwork face_detection::read_network()
    {
        LOG(INFO) << "Loading " << topoName << " model from: " << pathToModel;

        CNNNetReader netReader;
        /** Read network model **/
        netReader.ReadNetwork( pathToModel );
        /** Set batch size **/
        //LOG(DEBUG) << "Batch size is set to " << maxBatch;
        netReader.getNetwork().setBatchSize( maxBatch );

        /** Extract model name and load its weights **/
        std::string binFileName = remove_ext( pathToModel ) + ".bin";
        netReader.ReadWeights( binFileName );

        /** SSD-based network should have one input and one output **/

        InputsDataMap inputInfo( netReader.getNetwork().getInputsInfo() );
        if( inputInfo.size() != 1 )
            throw std::logic_error( "Face Detection network should have only one input" );
        _input_layer_name = inputInfo.begin()->first;
        InputInfo::Ptr inputInfoFirst = inputInfo.begin()->second;
        inputInfoFirst->setPrecision( Precision::U8 );

        OutputsDataMap outputInfo( netReader.getNetwork().getOutputsInfo() );
        if( outputInfo.size() != 1 )
            throw std::logic_error(
                "Face Detection network should have only one output" );
        _output_layer_name = outputInfo.begin()->first;
        DataPtr & outputDataPtr = outputInfo.begin()->second;
        const CNNLayerPtr outputLayer = netReader.getNetwork().getLayerByName( _output_layer_name.c_str() );
        if( outputLayer->type != "DetectionOutput" )
            throw std::logic_error(
                "Face Detection network output layer(" + outputLayer->name +
                ") should be DetectionOutput, but was " + outputLayer->type );
        if( outputLayer->params.find( "num_classes" ) == outputLayer->params.end() )
            throw std::logic_error(
                "Face Detection network output layer (" +
                _output_layer_name + ") should have num_classes integer attribute" );

        /*
            Expect a blob of [1, 1, N, 7], where N is the number of detected bounding boxes.
            For each detection, the description has the format: [image_id, label, conf, x_min, y_min, x_max, y_max]
                image_id - ID of the image in the batch
                label - predicted class ID
                conf - confidence for the predicted class
                (x_min, y_min) - coordinates of the top left bounding box corner
                (x_max, y_max) - coordinates of the bottom right bounding box corner.
        */
        const SizeVector & outputDims = outputDataPtr->getTensorDesc().getDims();
        if( outputDims.size() != 4 )
            throw std::logic_error(
                "Face Detection network output dimensions should be 4, but was " + std::to_string( outputDims.size() ) );
        size_t objectSize = outputDims[3];
        if( objectSize != DETECTED_OBJECT_SIZE )
            throw std::logic_error(
                "Face Detection network output layer last dimension should be " +
                std::to_string( DETECTED_OBJECT_SIZE ) + "; got " + std::to_string( objectSize ) );
        _max_results = outputDims[2];
        outputDataPtr->setPrecision( Precision::FP32 );

        return netReader.getNetwork();
    }


    std::vector< face_detection::Result > face_detection::fetch_results()
    {
        std::vector< Result > results;
        const float *detections = _request->GetBlob( _output_layer_name )->buffer().as<float *>();

        for( size_t i = 0; i < _max_results; i++ )
        {
            float image_id = detections[i * DETECTED_OBJECT_SIZE + 0];
            if( image_id < 0 )
                break;

            // [image_id, label, confidence, x_min, y_min, x_max, y_max]
            Result r;
            r.label = static_cast<int>(detections[i * DETECTED_OBJECT_SIZE + 1]);
            r.confidence = detections[i * DETECTED_OBJECT_SIZE + 2];
            if( r.confidence <= _detection_threshold && !doRawOutputMessages )
                continue;
            r.location.x = static_cast<int>(detections[i * DETECTED_OBJECT_SIZE + 3] * _width);
            r.location.y = static_cast<int>(detections[i * DETECTED_OBJECT_SIZE + 4] * _height);
            r.location.width = static_cast<int>(detections[i * DETECTED_OBJECT_SIZE + 5] * _width - r.location.x);
            r.location.height = static_cast<int>(detections[i * DETECTED_OBJECT_SIZE + 6] * _height - r.location.y);

            if( doRawOutputMessages )
            {
                LOG(DEBUG)
                    << "[" << i << "," << r.label << "] element, prob = " << r.confidence
                    << "    (" << r.location.x << "," << r.location.y << ")-(" << r.location.width << ","
                    << r.location.height << ")"
                    << ((r.confidence > _detection_threshold) ? " WILL BE RENDERED!" : "");
            }

            if( r.confidence > _detection_threshold )
                results.push_back( r );
        }

        return results;
    }
}
