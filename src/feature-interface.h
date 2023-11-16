// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <map>
#include <memory>


namespace librealsense {


// Base class for all features
class feature_interface
{
public:
    feature_interface( const std::string& id ) : _id( id )
    {
    }

    const std::string & get_id() const { return _id; }

    virtual ~feature_interface() = default;

private:
    std::string _id;
};

typedef std::map< std::string, std::shared_ptr< feature_interface > > features;

// Interface for objects that has features
class features_support
{
public:
    virtual bool supports_feature( const std::string & id ) const = 0;
    virtual std::shared_ptr< feature_interface > get_feature( const std::string & id ) = 0;
    virtual features get_supported_features() = 0;
};

}  // namespace librealsense
