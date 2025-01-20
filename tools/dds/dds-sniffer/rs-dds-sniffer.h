// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <string>
#include <mutex>
#include <set>

#include <realdds/dds-participant.h>

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>

class dds_sniffer
{
public:
    dds_sniffer();
    ~dds_sniffer();

    void print_discoveries( bool enable ) { _print_discoveries = enable; }
    void print_machine_readable( bool enable ) { _print_machine_readable = enable; }
    void print_topic_samples( bool enable ) { _print_topic_samples = enable; }

    void set_root( std::string const & root ) { _root = root; }

    bool init( realdds::dds_domain_id domain = 0 );

    realdds::dds_participant const & get_participant() const { return _participant; }

    void print_participants( bool with_topics = false, bool with_guids = false ) const;
    void print_topics() const;
    void print_topics_for( realdds::dds_guid_prefix, size_t indentation = 0 ) const;
    void print_topics_machine_readable() const;

private:
    std::shared_ptr< realdds::dds_participant::listener > _listener;
    realdds::dds_participant _participant;

    // For listing entities in the domain
    std::map< realdds::dds_guid, std::string > _discovered_participants;
    struct topic_info
    {
        std::string type_info;
        std::set< realdds::dds_guid > readers;
        std::set< realdds::dds_guid > writers;
    };
    std::map< std::string, topic_info > _topics_info_by_name;

    bool _print_discoveries = false;
    bool _print_machine_readable = false;
    bool _print_topic_samples = false;

    std::string _root;

    mutable std::mutex _dds_entities_lock;

    // For sniffing topics (can sniff only topics that send TypeObject during discovery)
    eprosima::fastdds::dds::Subscriber * _discovered_types_subscriber = nullptr;
    std::map< eprosima::fastdds::dds::DataReader *,
              eprosima::fastdds::dds::Topic * > _discovered_types_readers; // Save readers and topics for resource cleanup
    std::map< eprosima::fastdds::dds::DataReader *,
              eprosima::fastrtps::types::DynamicData_ptr > _discovered_types_datas; // Save allocated data buffer for sample read

    struct dds_reader_listener : public eprosima::fastdds::dds::DataReaderListener
    {
        dds_reader_listener( std::map< eprosima::fastdds::dds::DataReader *,
                             eprosima::fastrtps::types::DynamicData_ptr > & datas );

        void on_data_available( eprosima::fastdds::dds::DataReader * reader ) override;
        void on_subscription_matched( eprosima::fastdds::dds::DataReader *,
                                      const eprosima::fastdds::dds::SubscriptionMatchedStatus & info ) override;

    private:
        std::map< eprosima::fastdds::dds::DataReader *, eprosima::fastrtps::types::DynamicData_ptr > & _datas;
    };

    dds_reader_listener _reader_listener;  // define only after _discovered_types_datas (creation order matters)

    // Callbacks for dds-participant
    void on_writer_added( eprosima::fastrtps::rtps::WriterProxyData const & );
    void on_writer_removed( realdds::dds_guid guid, const char * topic_name );
    void on_reader_added( eprosima::fastrtps::rtps::ReaderProxyData const & );
    void on_reader_removed( realdds::dds_guid guid, const char * topic_name );
    void on_participant_added( realdds::dds_guid guid, const char * topic_name );
    void on_participant_removed( realdds::dds_guid guid, const char * topic_name );
    void on_type_discovery( char const * topic_name, eprosima::fastrtps::types::DynamicType_ptr dyn_type );

    // Topics data-base functions
    void save_topic_writer( eprosima::fastrtps::rtps::WriterProxyData const & );
    void remove_topic_writer( realdds::dds_guid guid, const char * topic_name );
    void save_topic_reader( eprosima::fastrtps::rtps::ReaderProxyData const & );
    void remove_topic_reader( realdds::dds_guid guid, const char * topic_name );

    struct topic_display_stats
    {
        size_t max_indent;
        size_t max_len;
    };
    topic_display_stats calc_topic_display_stats() const;

    // Helper print functions
    void print_writer_discovered( realdds::dds_guid guid, const char * topic_name, bool discovered ) const;
    void print_reader_discovered( realdds::dds_guid guid, const char * topic_name, bool discovered ) const;
    void print_participant_discovered( realdds::dds_guid guid,
                                       const char * participant_name,
                                       bool discovered ) const;
    void ident( size_t indentation, char const * prefix = "- " ) const;
    void print_topic_writer( realdds::dds_guid writer, size_t indentation = 0 ) const;
    void print_topic_reader( realdds::dds_guid reader, size_t indentation = 0 ) const;
    void print_topic_info( bool is_leaf,
                           size_t indentation,
                           std::string const & name,
                           topic_info const &,
                           topic_display_stats const &,
                           char const * prefix = "- " ) const;
};
