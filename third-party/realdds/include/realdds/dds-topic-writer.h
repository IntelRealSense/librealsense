// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include "dds-defines.h"

#include <rsutils/json-fwd.h>
#include <memory>
#include <atomic>


namespace eprosima {
namespace fastdds {
namespace dds {
class Publisher;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_topic;
class dds_publisher;


// The 'writer' is used to write data to a topic, bound at creation time (and therefore bound to a specific type).
// 
// You may choose to create one via a 'publisher' that manages the activities of several writers and determines when the
// data is actually sent. By default, data is sent as soon as the writer’s write() function is called.
//
class dds_topic_writer : protected eprosima::fastdds::dds::DataWriterListener
{
    std::shared_ptr< dds_topic > const _topic;
    std::shared_ptr< dds_publisher > const _publisher;

    eprosima::fastdds::dds::DataWriter * _writer = nullptr;

    std::atomic< int > _n_readers;

public:
    dds_topic_writer( std::shared_ptr< dds_topic > const & topic );
    dds_topic_writer( std::shared_ptr< dds_topic > const & topic, std::shared_ptr< dds_publisher > const & publisher );
    ~dds_topic_writer();

    eprosima::fastdds::dds::DataWriter * get() const { return _writer; }
    eprosima::fastdds::dds::DataWriter * operator->() const { return get(); }

    bool is_running() const { return ( get() != nullptr ); }
    bool has_readers() const { return _n_readers > 0; }

    std::shared_ptr< dds_topic > const & topic() const { return _topic; }
    std::shared_ptr< dds_publisher > const & publisher() const { return _publisher; }
    dds_guid const & guid() const;

    typedef std::function< void( eprosima::fastdds::dds::PublicationMatchedStatus const & ) >
        on_publication_matched_callback;
    void on_publication_matched( on_publication_matched_callback callback )
    {
        _on_publication_matched = std::move( callback );
    }

    class qos : public eprosima::fastdds::dds::DataWriterQos
    {
        using super = eprosima::fastdds::dds::DataWriterQos;

    public:
        qos( eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability
               = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS,  // default
             eprosima::fastdds::dds::DurabilityQosPolicyKind durability
               = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS );  // default is transient local
    
        // Override default values with JSON contents
        void override_from_json( rsutils::json const & );
    };

    // The callbacks should be set before we actually create the underlying DDS objects, so the writer does not
    void run( qos const & = qos() );

    // Waits until readers are detected; return false on timeout
    bool wait_for_readers( dds_time timeout );

    // Waits until all changes were acknowledged; return false on timeout
    bool wait_for_acks( dds_time timeout );

    // DataWriterListener
protected:
    // Called when the Publisher is matched (or unmatched) against an endpoint
    void on_publication_matched( eprosima::fastdds::dds::DataWriter * writer,
                                 eprosima::fastdds::dds::PublicationMatchedStatus const & info ) override;

private:
    on_publication_matched_callback _on_publication_matched;
};


}  // namespace realdds
