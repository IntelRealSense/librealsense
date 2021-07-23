// Copyright (C) 2020 Intel Corporation
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
        bool isAsync,
        int maxBatch, bool isBatchDynamic,
        bool doRawOutputMessages
    )
        : base_detection( "age/gender", pathToModel, maxBatch, isBatchDynamic, isAsync, doRawOutputMessages)
        , _n_enqued_frames(0)
    {
    }


    void age_gender_detection::submit_request()
    {
        if( !_n_enqued_frames )
            return;
        _n_enqued_frames = 0;
        base_detection::submit_request();
    }


    void age_gender_detection::enqueue( const cv::Mat &face )
    {
        if( !enabled() )
            return;
        if( !_request )
            _request = net.CreateInferRequestPtr();

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
        LOG(INFO) << "Loading " << topoName << " model from: " << pathToModel;

        CNNNetwork network;

#ifdef OPENVINO2019
        CNNNetReader netReader;

        /** Read network model **/
        netReader.ReadNetwork( pathToModel );
        network = netReader.getNetwork();

        /** Extract model name and load its weights **/
        std::string binFileName = remove_ext( pathToModel ) + ".bin";
        netReader.ReadWeights( binFileName );
#else
        InferenceEngine::Core ie;
        /** Read network model **/
        network = ie.ReadNetwork(pathToModel);
#endif

        /** Set batch size **/
        //LOG(DEBUG) << "Batch size is set to " << maxBatch;
        network.setBatchSize(maxBatch);

        // Age/Gender Recognition network should have one input and two outputs

        LOG(DEBUG) << "Checking Age/Gender Recognition network inputs";
        InputsDataMap inputInfo(network.getInputsInfo());
        if (inputInfo.size() != 1)
            throw std::logic_error("Age/Gender Recognition network should have only one input");
        InputInfo::Ptr& inputInfoFirst = inputInfo.begin()->second;
        inputInfoFirst->setPrecision(Precision::U8);
        input = inputInfo.begin()->first;

        LOG(DEBUG) << "Checking Age/Gender Recognition network outputs";
        OutputsDataMap outputInfo(network.getOutputsInfo());
        if (outputInfo.size() != 2)
            throw std::logic_error("Age/Gender Recognition network should have two output layers");
        auto it = outputInfo.begin();

        DataPtr ptrAgeOutput = (it++)->second;
        DataPtr ptrGenderOutput = (it++)->second;

        if (!ptrAgeOutput)
            throw std::logic_error("Age output data pointer is not valid");
        if (!ptrGenderOutput)
            throw std::logic_error("Gender output data pointer is not valid");


#ifdef OPENVINO2019
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

        if (doRawOutputMessages)
        {
            LOG(DEBUG) << "Age layer: " << ageCreatorLayer->name;
            LOG(DEBUG) << "Gender layer: " << genderCreatorLayer->name;
        }
#else
#ifdef OPENVINO_NGRAPH
        if (auto ngraphFunction = network.getFunction())
        {
            // Looking for the age and gender nodes in the ngraph: the age layer node should be Convolution type.
            // If we find ptrGenderOutput is with Convolution type, swap them.
            for (const auto& op : ngraphFunction->get_ops())
            {
                std::string friendly_name = op->get_friendly_name();
                std::string output_type = op->get_type_name();

                if ((friendly_name.find(ptrGenderOutput->getName()) != std::string::npos) && (output_type == "Convolution"))
                {
                    std::swap(ptrAgeOutput, ptrGenderOutput);
                    break;
                }
            }

            bool outputAgeOk = false;

            for (const auto& op : ngraphFunction->get_ops())
            {
                std::string friendly_name = op->get_friendly_name();
                std::string output_type = op->get_type_name();

                if ((friendly_name.find(ptrAgeOutput->getName()) != std::string::npos) && (output_type == "Convolution")) {
                    outputAgeOk = true;
                    break;
                }
            }

            if (!outputAgeOk)
            {
                throw std::logic_error("In Age/Gender Recognition network, Age layer (" + ptrAgeOutput->getName() + ") should be a Convolution");
            }

            bool outputGenderOk = false;

            for (const auto& op : ngraphFunction->get_ops()) {
                std::string friendly_name = op->get_friendly_name();
                std::string output_type = op->get_type_name();

                if ((friendly_name.find(ptrGenderOutput->getName()) != std::string::npos) && (output_type == "Softmax")) {
                    outputGenderOk = true;
                    break;
                }
            }

            if (!outputGenderOk)
            {
                throw std::logic_error("In Age/Gender Recognition network, Gender layer (" + ptrGenderOutput->getName() + ") should be a Softmax");
            }
        }

        if (doRawOutputMessages)
        {
            LOG(DEBUG) << "Age layer: " << ptrAgeOutput->getName();
            LOG(DEBUG) << "Gender layer: " << ptrGenderOutput->getName();
        }
#endif
#endif

        outputAge = ptrAgeOutput->getName();
        outputGender = ptrGenderOutput->getName();

        _enabled = true;
        return network;
    }
}
