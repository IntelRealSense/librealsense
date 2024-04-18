// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

#include <rsutils/json-fwd.h>
#include <functional>
#include <memory>


namespace eprosima {
namespace fastdds {
namespace dds {
class Subscriber;
}  // namespace dds
}  // namespace fastdds
}  // namespace eprosima


namespace realdds {


class dds_topic;
class dds_subscriber;

// The 'reader' is the entity used to subscribe to updated values of data in a topic. It is bound at creation to this
// topic.
// 
// You may choose to create one via a 'subscriber' that manages the activities of several readers.
// on_data_available callback will be called when a sample is received.
//
class dds_topic_reader : public eprosima::fastdds::dds::DataReaderListener
{
protected:
    std::shared_ptr< dds_topic > const _topic;
    std::shared_ptr < dds_subscriber > const _subscriber;

    eprosima::fastdds::dds::DataReader * _reader = nullptr;

    int _n_writers = 0;

public:
    dds_topic_reader( std::shared_ptr< dds_topic > const & topic );
    dds_topic_reader( std::shared_ptr< dds_topic > const & topic, std::shared_ptr< dds_subscriber > const & subscriber );
    virtual ~dds_topic_reader();

    eprosima::fastdds::dds::DataReader * get() const { return _reader; }
    eprosima::fastdds::dds::DataReader * operator->() const { return get(); }

    bool is_running() const { return ( get() != nullptr ); }
    bool has_writers() const { return _n_writers > 0; }

    std::shared_ptr< dds_topic > const & topic() const { return _topic; }

    typedef std::function< void() > on_data_available_callback;
    typedef std::function< void( eprosima::fastdds::dds::SubscriptionMatchedStatus const & ) >
        on_subscription_matched_callback;
    typedef std::function< void( eprosima::fastdds::dds::SampleLostStatus const & ) >
        on_sample_lost_callback;

    void on_data_available( on_data_available_callback callback ) { _on_data_available = std::move( callback ); }
    void on_subscription_matched( on_subscription_matched_callback callback )
    {
        _on_subscription_matched = std::move( callback );
    }
    void on_sample_lost( on_sample_lost_callback callback )
    {
        _on_sample_lost = std::move( callback );
    }

    class qos : public eprosima::fastdds::dds::DataReaderQos
    {
        using super = eprosima::fastdds::dds::DataReaderQos;

    public:
        qos( eprosima::fastdds::dds::ReliabilityQosPolicyKind reliability
               = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS,  // default
             eprosima::fastdds::dds::DurabilityQosPolicyKind durability
               = eprosima::fastdds::dds::VOLATILE_DURABILITY_QOS );  // default is transient local

        // Override default values with JSON contents
        void override_from_json( rsutils::json const & );
    };

    // The callbacks should be set before we actually create the underlying DDS objects, so the reader does not
    virtual void run( qos const & );

    // Go back to a pre-run() state, such that is_running() returns false
    virtual void stop();

    // DataReaderListener
protected:
    void on_subscription_matched( eprosima::fastdds::dds::DataReader *,
                                  eprosima::fastdds::dds::SubscriptionMatchedStatus const & info ) override;

    void on_data_available( eprosima::fastdds::dds::DataReader * ) override;

    void on_sample_lost( eprosima::fastdds::dds::DataReader *, const eprosima::fastdds::dds::SampleLostStatus & ) override;

protected:
    on_data_available_callback _on_data_available;
    on_subscription_matched_callback _on_subscription_matched;
    on_sample_lost_callback _on_sample_lost;
};


}  // namespace realdds
