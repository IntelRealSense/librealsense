// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <string>
#include <mutex>
#include <set>

#include <librealsense2/dds/dds-participant.h>

class dds_sniffer
{
public:
    dds_sniffer();
    ~dds_sniffer();

    bool init( librealsense::dds::dds_domain_id domain = 0, bool snapshot = false, bool machine_readable = false );
    void run( uint32_t seconds );

private:
    std::shared_ptr< librealsense::dds::dds_participant::listener > _listener;
    librealsense::dds::dds_participant _participant;

    std::map< librealsense::dds::dds_guid, std::string> _discovered_participants;
    struct topic_info
    {
        std::set< librealsense::dds::dds_guid > readers;
        std::set< librealsense::dds::dds_guid > writers;
    };
    std::map< std::string, topic_info > _discovered_topics;

    bool _print_discoveries = false;
    bool _print_by_topics = false;
    bool _print_machine_readable = false;

    mutable std::mutex _dds_entities_lock;

    //Callbacks for dds-participant
    void on_writer_added( librealsense::dds::dds_guid guid, const char* topic_name );
    void on_writer_removed( librealsense::dds::dds_guid guid, const char* topic_name );
    void on_reader_added( librealsense::dds::dds_guid guid, const char* topic_name );
    void on_reader_removed( librealsense::dds::dds_guid guid, const char* topic_name );
    void on_participant_added( librealsense::dds::dds_guid guid, const char* topic_name );
    void on_participant_removed( librealsense::dds::dds_guid guid, const char* topic_name );

    // Topics data-base functions
    void save_topic_writer( librealsense::dds::dds_guid guid, const char* topic_name );
    void remove_topic_writer( librealsense::dds::dds_guid guid, const char* topic_name );
    void save_topic_reader( librealsense::dds::dds_guid guid, const char* topic_name );
    void remove_topic_reader( librealsense::dds::dds_guid guid, const char* topic_name );
    uint32_t calc_max_indentation() const;

    // Helper print functions
    void print_writer_discovered( librealsense::dds::dds_guid guid, const char* topic_name, bool discovered ) const;
    void print_reader_discovered( librealsense::dds::dds_guid guid, const char* topic_name, bool discovered ) const;
    void print_participant_discovered( librealsense::dds::dds_guid guid, const char* participant_name, bool discovered ) const;
    void print_topics_machine_readable() const;
    void print_topics() const;
    void ident( uint32_t indentation ) const;
    void print_topic_writer( librealsense::dds::dds_guid writer, uint32_t indentation = 0) const;
    void print_topic_reader( librealsense::dds::dds_guid reader, uint32_t indentation = 0) const;
};
