// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

// The following warnings are given when including the OpenVINO headers:
//     ...\openvino\inference_engine\include\ie_layouts.h(127): warning C4251: 'InferenceEngine::BlockingDesc::blockedDims': class 'std::vector<size_t,std::allocator<_Ty>>' needs to have dll-interface to be used by clients of class 'InferenceEngine::BlockingDesc'
// And:
//     ...\openvino\inference_engine\src\extension\ext_list.hpp(25): warning C4275: non dll-interface class 'InferenceEngine::IExtension' used as base for dll-interface class 'InferenceEngine::Extensions::Cpu::CpuExtensions' (compiling source file ...)
// These should be harmless and not affect us. We disable them:
// (They even disable these warnings in their own CMake... see inference_engine/src/extension/CMakeList.txt)
#pragma warning(push)
#pragma warning(disable:4251)
#pragma warning(disable:4275)
#   include <inference_engine.hpp>
#   include <ie_iextension.h>
#   include <ext_list.hpp>              // Required for CPU extension usage
#pragma warning(pop)

#include <opencv2/opencv.hpp>
#include <easylogging++.h>


namespace openvino_helpers
{
    /*
        Sets image data stored in cv::Mat object to a given Blob object.
        Copies the mat data into the blob.
    */
    template <typename T>
    void matU8ToBlob( const cv::Mat& orig_image, InferenceEngine::Blob::Ptr& blob, int batchIndex = 0 )
    {
        InferenceEngine::SizeVector blobSize = blob->getTensorDesc().getDims();
        const int width = int( blobSize[3] );
        const int height = int( blobSize[2] );
        const size_t channels = blobSize[1];
        T* blob_data = blob->buffer().as<T*>();

        cv::Mat resized_image( orig_image );
        if( static_cast<int>(width) != orig_image.size().width ||
            static_cast<int>(height) != orig_image.size().height )
        {
            cv::resize( orig_image, resized_image, cv::Size( width, height ) );
        }

        size_t batchOffset = batchIndex * width * height * channels;

        if( channels == 1 )
        {
            for( int h = 0; h < height; h++ )
            {
                for( int w = 0; w < width; w++ )
                {
                    blob_data[batchOffset + h * width + w] = resized_image.at<uchar>( h, w );
                }
            }
        }
        else if( channels == 3 )
        {
            for( int c = 0; c < channels; c++ )
            {
                for( int h = 0; h < height; h++ )
                {
                    for( int w = 0; w < width; w++ )
                    {
                        blob_data[batchOffset + c * width * height + h * width + w] =
                            resized_image.at<cv::Vec3b>( h, w )[c];
                    }
                }
            }
        }
        else
        {
            THROW_IE_EXCEPTION << "Unsupported number of channels";
        }
    }


    /*
        Wraps data stored inside of a passed cv::Mat object by new Blob pointer.
        No memory allocation occurs. The blob just points to existing cv::Mat data.
    */
    static InferenceEngine::Blob::Ptr wrapMat2Blob( const cv::Mat &mat )
    {
        size_t channels = mat.channels();
        size_t height = mat.size().height;
        size_t width = mat.size().width;

        size_t strideH = mat.step.buf[0];
        size_t strideW = mat.step.buf[1];

        bool is_dense =
            strideW == channels &&
            strideH == channels * width;

        if( !is_dense ) THROW_IE_EXCEPTION
            << "Doesn't support conversion from not dense cv::Mat";

        InferenceEngine::TensorDesc tDesc( InferenceEngine::Precision::U8,
            { 1, channels, height, width },
            InferenceEngine::Layout::NHWC );

        return InferenceEngine::make_shared_blob<uint8_t>( tDesc, mat.data );
    }


    /*
        Remove extension from a file name.
    */
    inline std::string remove_ext( const std::string & filepath )
    {
        auto pos = filepath.rfind( '.' );
        if( pos == std::string::npos )
            return filepath;
        return filepath.substr( 0, pos );
    }


    /*
        Calculate the mean intensity of the given image
    */
    inline float calc_intensity( const cv::Mat & src )
    {
        cv::Mat tmp;
        cv::cvtColor( src, tmp, cv::COLOR_RGB2GRAY );
        cv::Scalar mean = cv::mean( tmp );

        return static_cast<float>(mean[0]);
    }


    /*
    */
    inline std::vector< std::string > read_labels( std::string const & filename )
    {
        std::vector< std::string > labels;
        std::ifstream inputFile( filename );
        std::copy( std::istream_iterator< std::string >( inputFile ),
            std::istream_iterator< std::string >(),
            std::back_inserter( labels ) );
        return labels;
    }


    /*
        Allow manipulation of a face bounding box so as to make additional face analytic networks more
        effective.

        For example, a face may not include the hair. Gender detection, though, may find the hair very
        important for proper classification!
    */
    inline cv::Rect adjust_face_bbox(
        cv::Rect const & r,
        float enlarge_coefficient = 1,
        float dx_coefficient = 1,
        float dy_coefficient = 1
    )
    {
        int w = r.width;
        int h = r.height;
        int center_x = r.x + w / 2;
        int center_y = r.y + h / 2;

        // Make square and enlarge
        int max_of_sizes = std::max( w, h );
        int new_width = static_cast<int>(enlarge_coefficient * max_of_sizes);
        int new_height = static_cast<int>(enlarge_coefficient * max_of_sizes);

        // Offset, if requested
        int new_x = center_x - static_cast<int>(std::floor( dx_coefficient * new_width / 2 ));
        int new_y = center_y - static_cast<int>(std::floor( dy_coefficient * new_height / 2 ));

        return cv::Rect( new_x, new_y, new_width, new_height );
    }


    /*
        Implementation of OpenVINO interface, allowing us to listen to any errors that occur
        and output them for debugging using LOG(DEBUG).

        Example usage:
            InferenceEngine::Core engine;
            openvino_helpers::error_listener error_listener;
            engine.SetLogCallback( error_listener );
    */
    class error_listener : public InferenceEngine::IErrorListener
    {
        void onError( char const * msg ) noexcept override
        {
            LOG(DEBUG) << "[InferenceEngine] " << msg;
        }
    };
}
