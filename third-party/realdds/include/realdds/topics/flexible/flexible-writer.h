// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <realdds/topics/flexible-msg.h>

#include <rsutils/json-fwd.h>
#include <string>
#include <memory>
#include <chrono>


namespace eprosima {
namespace fastdds {
namespace dds {
struct PublicationMatchedStatus;
}
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_participant;
class dds_topic;
class dds_topic_writer;


namespace topics {


class flexible_writer
{
    std::shared_ptr< dds_topic_writer > _writer;
    int _n_readers = 0;

public:
    flexible_writer( std::shared_ptr< dds_topic > const & topic );
    flexible_writer( std::shared_ptr< dds_participant > const & participant, std::string const & topic_name )
        : flexible_writer( flexible_msg::create_topic( participant, topic_name ) )
    {
    }

    std::string const & name() const;

    void wait_for_readers( int n_readers, std::chrono::seconds timeout = std::chrono::seconds( 3 ) );

    void write( rsutils::json && json );

private:
    void on_publication_matched( eprosima::fastdds::dds::PublicationMatchedStatus const & status );
};


}  // namespace topics
}  // namespace realdds
