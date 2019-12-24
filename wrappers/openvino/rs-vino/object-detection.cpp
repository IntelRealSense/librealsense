// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.


#include <rs-vino/object-detection.h>
#include <rs-vino/openvino-helpers.h>
#include <easylogging++.h>


using namespace InferenceEngine;


const size_t DETECTED_OBJECT_SIZE = 7;   // the size of each detected object


namespace openvino_helpers
{
    object_detection::object_detection(
        const std::string &pathToModel,
        double detectionThreshold,
        bool isAsync,
        int maxBatch, bool isBatchDynamic,
        bool doRawOutputMessages
    )
        : base_detection( "object detection", pathToModel, maxBatch, isBatchDynamic, isAsync, doRawOutputMessages )
        , _detection_threshold( detectionThreshold )
        , _max_results( 0 )
        , _n_enqued_frames( 0 ), _width( 0 ), _height( 0 )
    {
    }


    void object_detection::submit_request()
    {
        if( !_n_enqued_frames )
            return;
        _n_enqued_frames = 0;
        base_detection::submit_request();
    }


    void object_detection::enqueue( const cv::Mat & frame )
    {
        if( !enabled() )
            return;

        if( !_request )
            _request = net.CreateInferRequestPtr();

        _width = static_cast<float>(frame.cols);
        _height = static_cast<float>(frame.rows);

        Blob::Ptr  inputBlob = _request->GetBlob( _input_layer_name );
        matU8ToBlob<uint8_t>( frame, inputBlob );

        if( ! _im_info_name.empty() )
        {
            Blob::Ptr infoBlob = _request->GetBlob( _im_info_name );

            // (height, width, image_scale)
            float * p = infoBlob->buffer().as< PrecisionTrait< Precision::FP32 >::value_type * >();
            p[0] = static_cast< float >( _input_width );
            p[1] = static_cast< float >( _input_height );
            for( size_t k = 2; k < _im_info_size; k++ )
                p[k] = 1.f;  // all scale factors are set to 1.0
        }

        _n_enqued_frames = 1;
    }


    CNNNetwork object_detection::read_network()
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

        // We support networks with one or two inputs, though others may be possible...
        InputsDataMap inputInfo( netReader.getNetwork().getInputsInfo() );
        if( inputInfo.size() != 1  &&  inputInfo.size() != 2 )
            throw std::logic_error( "Object detection network should have only one or two inputs" );
        for( auto & item : inputInfo )
        {
            if( item.second->getInputData()->getTensorDesc().getDims().size() == 4 )
            {
                // Blob "data" (1x4) will contain the actual image data (e.g., 1,3,224,224 or 1,3,300,300)
                _input_layer_name = item.first;
                _input_width = item.second->getTensorDesc().getDims()[2];
                _input_height = item.second->getTensorDesc().getDims()[3];
                item.second->setPrecision( Precision::U8 );
            }
            else if( item.second->getInputData()->getTensorDesc().getDims().size() == 2 )
            {
                // Blob "im_info" is optional: 1x3 (height, width, image_scale)
                _im_info_name = item.first;
                auto const & dims = item.second->getTensorDesc().getDims();
                if( dims[0] != 1 )
                    throw std::logic_error( "Invalid input info: layer \"" + _im_info_name + "\" should be 1x3 or 1x6" );
                _im_info_size = dims[1];
                item.second->setPrecision( Precision::FP32 );
                if( _im_info_size != 3  &&  _im_info_size != 6 )
                    throw std::logic_error( "Invalid input info: layer \"" + _im_info_name + "\" should be 1x3 or 1x6" );
            }
        }
        if( _input_layer_name.empty() )
            throw std::logic_error( "Could not find input \"data\" layer in network" );

        // Only a single "DetectionOuput" layer is expected
        OutputsDataMap outputInfo( netReader.getNetwork().getOutputsInfo() );
        if( outputInfo.size() != 1 )
            throw std::logic_error(
                "Object detection network should have only one output" );
        _output_layer_name = outputInfo.begin()->first;
        DataPtr & outputDataPtr = outputInfo.begin()->second;
        const CNNLayerPtr outputLayer = netReader.getNetwork().getLayerByName( _output_layer_name.c_str() );
        if( outputLayer->type != "DetectionOutput" )
            throw std::logic_error(
                "Object detection network output layer(" + outputLayer->name +
                ") should be DetectionOutput, but was " + outputLayer->type );
        if( outputLayer->params.find( "num_classes" ) == outputLayer->params.end() )
            throw std::logic_error(
                "Object detection network output layer (" +
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
                "Object detection network output dimensions should be 4, but was " + std::to_string( outputDims.size() ) );
        size_t objectSize = outputDims[3];
        if( objectSize != DETECTED_OBJECT_SIZE )
            throw std::logic_error(
                "Object detection network output layer last dimension should be " +
                std::to_string( DETECTED_OBJECT_SIZE ) + "; got " + std::to_string( objectSize ) );
        _max_results = outputDims[2];
        outputDataPtr->setPrecision( Precision::FP32 );

        return netReader.getNetwork();
    }


    std::vector< object_detection::Result > object_detection::fetch_results()
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
