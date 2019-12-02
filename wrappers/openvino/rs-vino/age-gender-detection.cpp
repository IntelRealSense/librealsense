// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <rs-vino/age-gender-detection.h>
#include <rs-vino/openvino-helpers.h>
#include <easylogging++.h>


using namespace InferenceEngine;


namespace openvino_helpers
{
    age_gender_detection::age_gender_detection(
        const std::string &pathToModel,
        int maxBatch, bool isBatchDynamic, bool isAsync,
        bool doRawOutputMessages
    )
        : base_detection( "Age/Gender", pathToModel, maxBatch, isBatchDynamic, isAsync, doRawOutputMessages)
        , _n_enqued_frames(0)
    {
    }


    void age_gender_detection::submit_request()
    {
        if( !_n_enqued_frames )
            return;
        if (isBatchDynamic) {
            _request->SetBatch( _n_enqued_frames );
        }
        base_detection::submit_request();
        _n_enqued_frames = 0;
    }


    void age_gender_detection::enqueue( const cv::Mat &face )
    {
        if( !enabled() )
            return;
        if( _n_enqued_frames == maxBatch )
        {
            LOG(WARNING) << "Number of detected faces more than maximum (" << maxBatch << ") processed by Age/Gender Recognition network";
            return;
        }
        if( !_request )
        {
            _request = net.CreateInferRequestPtr();
        }

        Blob::Ptr inputBlob = _request->GetBlob( input );
        matU8ToBlob<uint8_t>( face, inputBlob, _n_enqued_frames );

        ++_n_enqued_frames;
    }


    age_gender_detection::Result age_gender_detection::operator[] (int idx) const
    {
        Blob::Ptr  genderBlob = _request->GetBlob( outputGender );
        Blob::Ptr  ageBlob = _request->GetBlob( outputAge );

        age_gender_detection::Result r = {
            ageBlob->buffer().as<float*>()[idx] * 100,
            genderBlob->buffer().as<float*>()[idx * 2 + 1]
        };
        if (doRawOutputMessages)
            LOG(DEBUG) << "element" << idx << ", male prob = " << r.maleProb << ", age = " << r.age;

        return r;
    }


    CNNNetwork age_gender_detection::read_network()
    {
        LOG(INFO) << "Loading network files for Age/Gender Recognition network from: " << pathToModel;
        CNNNetReader netReader;
        // Read network
        netReader.ReadNetwork(pathToModel);

        // Set maximum batch size to be used.
        netReader.getNetwork().setBatchSize(maxBatch);
        if( doRawOutputMessages )
            LOG(DEBUG) << "Batch size is set to " << netReader.getNetwork().getBatchSize() << " for Age/Gender Recognition network";


        // Extract model name and load its weights
        std::string binFileName = fileNameNoExt(pathToModel) + ".bin";
        netReader.ReadWeights(binFileName);

        // Age/Gender Recognition network should have one input and two outputs

        LOG(DEBUG) << "Checking Age/Gender Recognition network inputs";
        InputsDataMap inputInfo(netReader.getNetwork().getInputsInfo());
        if (inputInfo.size() != 1)
            throw std::logic_error("Age/Gender Recognition network should have only one input");
        InputInfo::Ptr& inputInfoFirst = inputInfo.begin()->second;
        inputInfoFirst->setPrecision(Precision::U8);
        input = inputInfo.begin()->first;

        LOG(DEBUG) << "Checking Age/Gender Recognition network outputs";
        OutputsDataMap outputInfo(netReader.getNetwork().getOutputsInfo());
        if (outputInfo.size() != 2)
            throw std::logic_error("Age/Gender Recognition network should have two output layers");
        auto it = outputInfo.begin();

        DataPtr ptrAgeOutput = (it++)->second;
        DataPtr ptrGenderOutput = (it++)->second;

        if (!ptrAgeOutput)
            throw std::logic_error("Age output data pointer is not valid");
        if (!ptrGenderOutput)
            throw std::logic_error("Gender output data pointer is not valid");

        auto genderCreatorLayer = ptrGenderOutput->getCreatorLayer().lock();
        auto ageCreatorLayer = ptrAgeOutput->getCreatorLayer().lock();

        if (!ageCreatorLayer)
            throw std::logic_error("Age creator layer pointer is not valid");
        if (!genderCreatorLayer)
            throw std::logic_error("Gender creator layer pointer is not valid");

        // if gender output is convolution, it can be swapped with age
        if (genderCreatorLayer->type == "Convolution")
            std::swap(ptrAgeOutput, ptrGenderOutput);

        if (ptrAgeOutput->getCreatorLayer().lock()->type != "Convolution")
            throw std::logic_error("In Age/Gender Recognition network, age layer (" + ageCreatorLayer->name +
                                    ") should be a Convolution, but was: " + ageCreatorLayer->type);

        if (ptrGenderOutput->getCreatorLayer().lock()->type != "SoftMax")
            throw std::logic_error("In Age/Gender Recognition network, gender layer (" + genderCreatorLayer->name +
                                    ") should be a SoftMax, but was: " + genderCreatorLayer->type);
        if( doRawOutputMessages )
        {
            LOG(DEBUG) << "Age layer: " << ageCreatorLayer->name;
            LOG(DEBUG) << "Gender layer: " << genderCreatorLayer->name;
        }

        outputAge = ptrAgeOutput->getName();
        outputGender = ptrGenderOutput->getName();

        _enabled = true;
        return netReader.getNetwork();
    }
}
