// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once


#include "../converter.hpp"


namespace rs2 {
namespace tools {
namespace converter {


inline std::string frame_to_string( rs2::frame const & f )
{
    std::ostringstream s;

    if( ! f )
    {
        s << "[null]";
    }
    else
    {
        rs2::frameset fs( f );
        if( fs )
        {
            s << "[";
            fs.foreach_rs( [&s]( rs2::frame const & sf ) { s << frame_to_string( sf ); } );
            s << "]";
        }
        else
        {
            auto p = f.get_profile();
            s << "[" << rs2_stream_to_string( p.stream_type() );
            s << "/" << p.unique_id(); //stream_index();
            s << " #" << f.get_frame_number();
            s << " @" << std::fixed << (double)f.get_timestamp();
            s << "]";
        }
    }
    return s.str();
}


class converter_text : public converter_base
{
    rs2_stream _streamType;

public:
    converter_text( rs2_stream streamType = rs2_stream::RS2_STREAM_ANY )
        : _streamType( streamType )
    {
    }

    std::string name() const override { return "TEXT converter"; }

    void convert( rs2::frame & frame ) override
    {
        std::cout << frame_to_string( frame ) << std::endl;
    }
};

}  // namespace converter
}  // namespace tools
}  // namespace rs2
