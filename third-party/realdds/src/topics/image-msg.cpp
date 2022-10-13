// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/image/imagePubSubTypes.h>

#include <realdds/dds-topic.h>
#include <realdds/dds-utilities.h>

#include <fastdds/dds/topic/Topic.hpp>


namespace realdds {
namespace topics {
namespace device {


image::image( const raw::device::image & image )
    : raw_data( image.raw_data() )  // TODO: avoid image copy?
    , width( image.width())
    , height( image.height() )
    , size( image.size() )
    , format( image.format() )
{
}


/*static*/ std::shared_ptr< dds_topic >
image::create_topic( std::shared_ptr< dds_participant > const & participant, char const * topic_name )
{
    return std::make_shared< dds_topic >( participant,
                                          eprosima::fastdds::dds::TypeSupport( new image::type ),
                                          topic_name );
}


}  // namespace device
}  // namespace topics
}  // namespace realdds
