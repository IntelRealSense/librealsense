// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/feature-interface.h>


namespace librealsense {


class features_container : public features_support
{
public:
    bool supports_feature( const std::string & id ) const override
    {
        return _features.find( id ) != _features.end();
    }

    std::shared_ptr< feature_interface > get_feature( const std::string & id ) override
    {
        auto it = _features.find( id );
        return ( it == _features.end() ? std::shared_ptr< feature_interface >( nullptr ) : it->second );
    }

    features get_supported_features() override
    {
        return _features;
    }

    void register_feature( const std::string id, std::shared_ptr< feature_interface > feature )
    {
        _features[id] = feature;
    }

protected:
    features _features;
};


}  // namespace librealsense
