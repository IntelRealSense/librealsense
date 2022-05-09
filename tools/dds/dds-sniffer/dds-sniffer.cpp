// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-sniffer.hpp"

#include <thread>
#include <memory>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>
#include <tclap/SwitchArg.h>

using namespace TCLAP;
constexpr uint8_t GUID_PROCESS_LOCATION = 4;

int main( int argc, char ** argv ) try
{
    eprosima::fastdds::dds::DomainId_t domain = 0;
    uint32_t seconds = 0;

    CmdLine cmd( "librealsense rs-dds-sniffer tool", ' ' );
    SwitchArg snapshot_arg( "s", "snapshot", "run momentarily taking a snapshot of the domain" );
    SwitchArg machine_readable_arg( "m", "machine-readable", "output entities in a way more suitable for automatic parsing" );
    ValueArg< eprosima::fastdds::dds::DomainId_t > domain_arg( "d",
                                                               "domain",
                                                               "select domain ID to listen on",
                                                               false,
                                                               0,
                                                               "0-232" );
    cmd.add( snapshot_arg );
    cmd.add( machine_readable_arg );
    cmd.add( domain_arg );
    cmd.parse( argc, argv );

    if( snapshot_arg.isSet() )
    {
        seconds = 3;
    }

    if( domain_arg.isSet() )
    {
        domain = domain_arg.getValue();
        if( domain > 232 )
        {
            std::cerr << "Invalid domain value, enter a value in the range [0, 232]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    Sniffer snif;
    if( snif.init( domain, snapshot_arg.isSet(), machine_readable_arg.isSet() ) )
    {
        snif.run( seconds );
    }
    else
    {
        std::cerr << "Initialization failure" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
catch( const TCLAP::ExitException & )
{
    std::cerr << "Undefined exception while parsing command line arguments" << std::endl;
    return EXIT_FAILURE;
}
catch( const TCLAP::ArgException & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

Sniffer::Sniffer()
    : _participant( nullptr )
    , _subscriber( nullptr )
    , _topics()
    , _readers()
    , _datas()
    , _subscriberAttributes()
    , _readerQoS()
{
}

Sniffer::~Sniffer()
{
    for( const auto & it : _topics )
    {
        if( _subscriber != nullptr )
        {
            _subscriber->delete_datareader( it.first );
        }
        _participant->delete_topic( it.second );
    }
    if( _subscriber != nullptr )
    {
        if( _participant != nullptr )
        {
            _participant->delete_subscriber( _subscriber );
        }
    }
    if( _participant != nullptr )
    {
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant( _participant );
    }

    _datas.clear();
    _topics.clear();
    _readers.clear();
}

bool Sniffer::init( eprosima::fastdds::dds::DomainId_t domain, bool snapshot, bool machine_readable )
{
    _print_discoveries = !snapshot;
    _print_by_topics = snapshot;
    _print_machine_readable = machine_readable;

    eprosima::fastdds::dds::DomainParticipantQos participantQoS;
    participantQoS.name( "rs-dds-sniffer" );
    _participant = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant( domain,
                                                                                                         participantQoS,
                                                                                                         this );
    if (!_print_machine_readable)
    {
        if (snapshot)
        {
            std::cout << "rs-dds-sniffer taking a snapshot of domain " << domain << std::endl;
        }
        else
        {
            std::cout << "rs-dds-sniffer listening on domain " << domain << std::endl;
        }
    }

    return _participant != nullptr;
}

void Sniffer::run( uint32_t seconds )
{
    if( seconds == 0 )
    {
        std::cin.ignore( std::numeric_limits< std::streamsize >::max() );
    }
    else
    {
        std::this_thread::sleep_for( std::chrono::seconds( seconds ) );
    }

    if( _print_by_topics )
    {
        if (_print_machine_readable)
        {
            print_topics_machine_readable();
        }
        else
        {
            print_topics();
        }
    }
}

void Sniffer::on_data_available( eprosima::fastdds::dds::DataReader * reader )
{
    auto dataIter = _datas.find( reader );

    if( dataIter != _datas.end() )
    {
        eprosima::fastrtps::types::DynamicData_ptr data = dataIter->second;
        eprosima::fastdds::dds::SampleInfo info;
        if( reader->take_next_sample( data.get(), &info ) == ReturnCode_t::RETCODE_OK )
        {
            if( info.instance_state == eprosima::fastdds::dds::ALIVE_INSTANCE_STATE )
            {
                eprosima::fastrtps::types::DynamicType_ptr type = _readers[reader];
                std::cout << "Received data of type " << type->get_name() << std::endl;
                eprosima::fastrtps::types::DynamicDataHelper::print( data );
            }
        }
    }
}

void Sniffer::on_subscription_matched( eprosima::fastdds::dds::DataReader * reader,
                                       const eprosima::fastdds::dds::SubscriptionMatchedStatus & info )
{
    if( info.current_count_change == 1 )
    {
        _matched = info.current_count;  // MatchedStatus::current_count represents number of writers matched for a
                                        // reader, total_count represents number of readers matched for a writer.
        std::cout << "A reader has been added to the domain" << std::endl;
    }
    else if( info.current_count_change == -1 )
    {
        _matched = info.current_count;  // MatchedStatus::current_count represents number of writers matched for a
                                        // reader, total_count represents number of readers matched for a writer.
        std::cout << "A reader has been removed from the domain" << std::endl;
    }
    else
    {
        std::cout << "Invalid current_count_change value " << info.current_count_change << std::endl;
    }
}

void Sniffer::on_type_discovery( eprosima::fastdds::dds::DomainParticipant *,
                                 const eprosima::fastrtps::rtps::SampleIdentity &,
                                 const eprosima::fastrtps::string_255 & topic_name,
                                 const eprosima::fastrtps::types::TypeIdentifier *,
                                 const eprosima::fastrtps::types::TypeObject *,
                                 eprosima::fastrtps::types::DynamicType_ptr dyn_type )
{
    eprosima::fastdds::dds::TypeSupport m_type( new eprosima::fastrtps::types::DynamicPubSubType( dyn_type ) );
    m_type.register_type( _participant );

    std::cout << "Discovered type: " << m_type->getName() << " from topic " << topic_name << std::endl;

    // Create a data reader for the topic
    if( _subscriber == nullptr )
    {
        _subscriber = _participant->create_subscriber( eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr );

        if( _subscriber == nullptr )
        {
            return;
        }
    }

    eprosima::fastdds::dds::Topic * topic = _participant->create_topic( topic_name.c_str(),
                                                                        m_type->getName(),
                                                                        eprosima::fastdds::dds::TOPIC_QOS_DEFAULT );
    if( topic == nullptr )
    {
        return;
    }

    eprosima::fastdds::dds::DataReader * reader = _subscriber->create_datareader( topic, _readerQoS, this );

    _topics[reader] = topic;
    _readers[reader] = dyn_type;
    eprosima::fastrtps::types::DynamicData_ptr data(eprosima::fastrtps::types::DynamicDataFactory::get_instance()->create_data( dyn_type ) );
    _datas[reader] = data;
}

void Sniffer::on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant *,
                                      eprosima::fastrtps::rtps::WriterDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
        if( _print_discoveries )
        {
            print_writer_discovered( info, true );
        }
        save_topic_writer( info );
        break;
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
        if( _print_discoveries )
        {
            print_writer_discovered( info, false );
        }

        remove_topic_writer( info );
        break;
    }
}

void Sniffer::on_subscriber_discovery( eprosima::fastdds::dds::DomainParticipant *,
                                       eprosima::fastrtps::rtps::ReaderDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER:
        if( _print_discoveries )
        {
            print_reader_discovered( info, true );
        }
        save_topic_reader( info );
        break;
    case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::REMOVED_READER:
        if( _print_discoveries )
        {
            print_reader_discovered( info, false );
        }
        remove_topic_reader( info );
        break;
    }
}

void Sniffer::on_participant_discovery( eprosima::fastdds::dds::DomainParticipant *,
                                        eprosima::fastrtps::rtps::ParticipantDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
        if( _print_discoveries )
        {
            print_participant_discovered( info, true );
        }
        _discoveredParticipants[info.info.m_guid] = info.info.m_participantName;
        break;
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
    case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
        if( _print_discoveries )
        {
            print_participant_discovered( info, false );
        }
        _discoveredParticipants.erase( info.info.m_guid );
        break;
    }
}

void Sniffer::on_type_information_received( eprosima::fastdds::dds::DomainParticipant *,
                                            const eprosima::fastrtps::string_255 topic_name,
                                            const eprosima::fastrtps::string_255 type_name,
                                            const eprosima::fastrtps::types::TypeInformation & type_information )
{
    std::cout << "Topic '" << topic_name << "' with type '" << type_name << "' received" << std::endl;
}

void Sniffer::save_topic_writer( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info )
{
    std::string topic_name = info.info.topicName().c_str();
    if( _discoveredTopics.find( topic_name ) == _discoveredTopics.end() )
    {
        _discoveredTopics.insert( { topic_name, *std::make_shared< ReadersWriters >() } );
    }

    _discoveredTopics[topic_name].writers.insert( info.info.guid() );
    save_max_indentation( std::move( topic_name ) );
}

void Sniffer::remove_topic_writer( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info )
{
    auto topic_entry = _discoveredTopics[info.info.topicName().c_str()];
    auto writer_iter = topic_entry.writers.find( info.info.guid() );
    if( writer_iter == topic_entry.writers.end() )
    {
        topic_entry.writers.erase( writer_iter );
        if( topic_entry.writers.empty() && topic_entry.readers.empty() )
        {
            _discoveredTopics.erase( info.info.topicName().c_str() );
        }
    }
}

void Sniffer::save_topic_reader( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info )
{
    std::string topic_name = info.info.topicName().c_str();
    if( _discoveredTopics.find( topic_name ) == _discoveredTopics.end() )
    {
        _discoveredTopics.insert( { topic_name, *std::make_shared< ReadersWriters >() } );
    }

    _discoveredTopics[topic_name].readers.insert( info.info.guid() );
    save_max_indentation( std::move( topic_name ) );
}

void Sniffer::save_max_indentation( const std::string && str )
{
    std::istringstream topic( str );
    std::string nested;
    uint32_t indentation = 0;
    while( std::getline( topic, nested, '/' ) )  // Use / as delimiter for nested topic names
    {
        ++indentation;
    }
    if( indentation > _max_indentation )
    {
        _max_indentation = indentation;
    }
}

void Sniffer::remove_topic_reader( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info )
{
    auto topic_entry = _discoveredTopics[info.info.topicName().c_str()];
    auto reader_iter = topic_entry.readers.find( info.info.guid() );
    if( reader_iter != topic_entry.readers.end() )
    {
        topic_entry.readers.erase( reader_iter );
        if( topic_entry.writers.empty() && topic_entry.readers.empty() )
        {
            _discoveredTopics.erase( info.info.topicName().c_str() );
        }
    }
}

void Sniffer::print_writer_discovered( const eprosima::fastrtps::rtps::WriterDiscoveryInfo & info, bool discovered ) const
{
    if (_print_machine_readable)
    {
        std::cout << "DataWriter," << info.info.guid() << "," << info.info.topicName() << "," << info.info.typeName()
                  << (discovered ? ",discovered" : ",removed") << std::endl;
    }
    else
    {
        std::cout << "DataWriter  " << info.info.guid() << " publishing topic '" << info.info.topicName() << "' of type '"
                  << info.info.typeName() << (discovered ? "' discovered" : "' removed") << std::endl;
    }
}

void Sniffer::print_reader_discovered( const eprosima::fastrtps::rtps::ReaderDiscoveryInfo & info, bool discovered ) const
{
    if (_print_machine_readable)
    {
        std::cout << "DataReader," << info.info.guid() << "," << info.info.topicName() << "," << info.info.typeName()
                  << (discovered ? ",discovered" : ",removed") << std::endl;
    }
    else
    {
        std::cout << "DataReader  " << info.info.guid() << " reading topic '" << info.info.topicName() << "' of type '"
                  << info.info.typeName() << (discovered ? "' discovered" : "' removed") << std::endl;
    }
}

void Sniffer::print_participant_discovered( const eprosima::fastrtps::rtps::ParticipantDiscoveryInfo & info, bool discovered ) const
{
    if (_print_machine_readable)
    {
        std::cout << "Participant," << info.info.m_guid << "," << info.info.m_participantName
                  << (discovered ? ",discovered" : ",removed") << std::endl;
    }
    else
    {
        uint16_t tmp( 0 );
        memcpy( &tmp, &info.info.m_guid.guidPrefix.value[GUID_PROCESS_LOCATION], sizeof( tmp ) );
        std::cout << "Participant " << info.info.m_guid << " " << info.info.m_participantName << "_" << std::hex << tmp
                  << std::dec << (discovered ? " discovered" : " removed") << std::endl;
    }
}

void Sniffer::print_topics_machine_readable()
{
    for (auto topic : _discoveredTopics)
    {
        for (auto writer : topic.second.writers)
        {
            std::cout << topic.first << ",";
            print_topic_writer( writer, _max_indentation );
        }
        for (auto reader : topic.second.readers)
        {
            std::cout << topic.first << ",";
            print_topic_reader( reader, _max_indentation );
        }
    }
}

void Sniffer::print_topics()
{
    std::istringstream last_topic( "" );
    std::string last_topic_nested;
    for( auto topic : _discoveredTopics )
    {
        std::cout << std::endl;

        std::istringstream current_topic( topic.first );  // Get topic name
        std::string current_topic_nested;
        uint32_t indentation = 0;

        // Compare to previous topic
        while( std::getline( last_topic, last_topic_nested, '/' ) )  // Use / as delimiter for nested topic names
        {
            if( std::getline( current_topic, current_topic_nested, '/' ) )
            {
                if( current_topic_nested.compare( last_topic_nested ) == 0 )
                {
                    ++indentation;  // Skip parts that are same as previous topic
                }
                else
                {
                    ident( indentation );
                    std::cout << current_topic_nested << std::endl;
                    ++indentation;
                    break;
                }
            }
        }

        // Print reminder of string
        while( std::getline( current_topic, current_topic_nested, '/' ) )
        {
            ident( indentation );
            std::cout << current_topic_nested << std::endl;
            ++indentation;
        }

        for( auto writer : topic.second.writers )
        {
            print_topic_writer( writer, _max_indentation );
        }
        for( auto reader : topic.second.readers )
        {
            print_topic_reader( reader, _max_indentation );
        }

        last_topic.clear();
        last_topic.str( topic.first );  // Save topic name for next iteration
        last_topic.seekg( 0, last_topic.beg );
    }
}

void Sniffer::ident( uint32_t indentation ) const
{
    while( indentation > 0 )
    {
        std::cout << "    ";
        --indentation;
    }
    std::cout << "- ";
}

void Sniffer::print_topic_writer( const eprosima::fastrtps::rtps::GUID_t & writer, uint32_t indentation ) const
{
    auto iter = _discoveredParticipants.begin();
    for( ; iter != _discoveredParticipants.end(); ++iter )
    {
        if( iter->first.guidPrefix == writer.guidPrefix )
        {
            uint16_t tmp;
            memcpy( &tmp, &iter->first.guidPrefix.value[GUID_PROCESS_LOCATION], sizeof( tmp ) );
            if (_print_machine_readable)
            {
                std::cout << "Writer," << iter->second << "_" << std::hex << std::setw(4) << std::setfill( '0' ) << tmp << std::dec << std::endl;
            }
            else
            {
                ident( indentation );
                std::cout << "Writer of \"" << iter->second << "_" << std::hex << std::setw(4) << std::setfill( '0' ) << tmp << std::dec << "\"" << std::endl;
            }
            break;
        }
    }
    if( iter == _discoveredParticipants.end() )
    {
        ident( indentation );
        std::cout << "Writer of unknown participant" << std::endl;
    }
}

void Sniffer::print_topic_reader( const eprosima::fastrtps::rtps::GUID_t & reader, uint32_t indentation ) const
{
    auto iter = _discoveredParticipants.begin();
    for( auto iter = _discoveredParticipants.begin(); iter != _discoveredParticipants.end(); ++iter )
    {
        if( iter->first.guidPrefix == reader.guidPrefix )
        {
            uint16_t tmp;
            memcpy( &tmp, &iter->first.guidPrefix.value[GUID_PROCESS_LOCATION], sizeof( tmp ) );
            if (_print_machine_readable)
            {
                std::cout << "Reader," << iter->second << "_" << std::hex << std::setw(4) << std::setfill( '0' ) << tmp << std::dec << std::endl;
            }
            else
            {
                ident( indentation );
                std::cout << "Reader of \"" << iter->second << "_" << std::hex << std::setw(4) << std::setfill( '0' ) << tmp << std::dec << "\"" << std::endl;
            }
            break;
        }
    }
    if( iter == _discoveredParticipants.end() )
    {
        ident( indentation );
        std::cout << "Reader of unknown participant" << std::endl;
    }
}
