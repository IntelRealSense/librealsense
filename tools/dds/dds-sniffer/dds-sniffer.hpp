// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <string>
#include <mutex>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantListener.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastrtps/attributes/SubscriberAttributes.h>
#include <fastdds/rtps/common/Guid.h>

class dds_sniffer : public eprosima::fastdds::dds::DomainParticipantListener
{
public:
    dds_sniffer();
    ~dds_sniffer();

    bool init( eprosima::fastdds::dds::DomainId_t domain = 0, bool snapshot = false, bool machine_readable = false );
    void run( uint32_t seconds );

private:
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Subscriber * _subscriber;

    std::map< eprosima::fastdds::dds::DataReader *, eprosima::fastdds::dds::Topic * > _topics;
    std::map< eprosima::fastdds::dds::DataReader *, eprosima::fastrtps::types::DynamicType_ptr > _readers;
    std::map< eprosima::fastdds::dds::DataReader *, eprosima::fastrtps::types::DynamicData_ptr > _datas;

    std::map< eprosima::fastrtps::rtps::GUID_t, eprosima::fastrtps::string_255 > _discovered_participants;
    struct topic_info
    {
        std::set< eprosima::fastrtps::rtps::GUID_t > readers;
        std::set< eprosima::fastrtps::rtps::GUID_t > writers;
    };
    std::map< std::string, topic_info > _discovered_topics;

    eprosima::fastrtps::SubscriberAttributes _subscriber_attributes;

    eprosima::fastdds::dds::DataReaderQos _reader_qos;

    std::atomic_int _matched = { 0 };
    bool _print_discoveries = false;
    bool _print_by_topics = false;
    bool _print_machine_readable = false;

    mutable std::mutex _dds_entities_lock;

    void on_data_available( eprosima::fastdds::dds::DataReader * reader ) override;

    void on_subscription_matched( eprosima::fastdds::dds::DataReader * reader,
                                  const eprosima::fastdds::dds::SubscriptionMatchedStatus & info ) override;

    /*!
     * Called when a new Participant is discovered, or a previously discovered participant changes its QOS or is removed.
     */
    void on_participant_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                   eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info ) override;

    /*!
     * Called when a new Subscriber is discovered, or a previously discovered subscriber changes its QOS or is removed.
     */
    void on_subscriber_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                  eprosima::fastrtps::rtps::ReaderDiscoveryInfo && info ) override;

    /*!
     * Called when a new Publisher is discovered, or a previously discovered publisher changes its QOS or is removed
     */
    void on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                 eprosima::fastrtps::rtps::WriterDiscoveryInfo && info ) override;

    /*!
     * Called when a participant discovers a new Type
     */
    void on_type_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                            const eprosima::fastrtps::rtps::SampleIdentity & request_sample_id,
                            const eprosima::fastrtps::string_255 & topic,
                            const eprosima::fastrtps::types::TypeIdentifier * identifier,
                            const eprosima::fastrtps::types::TypeObject * object,
                            eprosima::fastrtps::types::DynamicType_ptr dyn_type ) override;

    void on_type_information_received( eprosima::fastdds::dds::DomainParticipant * participant,
                                       const eprosima::fastrtps::string_255 topic_name,
                                       const eprosima::fastrtps::string_255 type_name,
                                       const eprosima::fastrtps::types::TypeInformation & type_information ) override;

    // Topics data-base functions
    void save_topic_writer( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info );
    void remove_topic_writer( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info );
    void save_topic_reader( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info );
    void remove_topic_reader( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info );
    uint32_t calc_max_indentation() const;

    // Helper print functions
    void print_writer_discovered( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info, bool discovered ) const;
    void print_reader_discovered( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info, bool discovered ) const;
    void print_participant_discovered( const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo & info, bool discovered ) const;
    void print_topics_machine_readable() const;
    void print_topics() const;
    void ident( uint32_t indentation ) const;
    void print_topic_writer( const eprosima::fastrtps::rtps::GUID_t & writer, uint32_t indentation = 0) const;
    void print_topic_reader( const eprosima::fastrtps::rtps::GUID_t & reader, uint32_t indentation = 0) const;
};