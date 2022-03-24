// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "dds-sniffer.hpp"

#include <thread>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/qos/DomainParticipantQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastrtps/types/DynamicDataHelper.hpp>
#include <fastrtps/types/DynamicDataFactory.h>

#include <fastrtps/transport/UDPv4TransportDescriptor.h>


int main(int argc, char** argv)
{
    eprosima::fastdds::dds::DomainId_t domain = 0;
    uint32_t seconds = 0;

    for(int i = 2; i <= argc; ++i)
    {
        if (strcmp(argv[i - 1], "--help") == 0 ||
            strcmp(argv[i - 1], "-h") == 0)
        {
            std::cout << "dds-sniffer tool\n"
                      << "\t-h --help: shows this menu\n" 
                      << "\t-d --domain <ID>: select domain ID to listen on\n" 
                      << "\t-s --snapshot: run momentarily taking a snapshot of the domain\n" << std::endl;

            return 0;
        }

        if (strcmp(argv[i - 1], "--snapshot") == 0 ||
            strcmp(argv[i - 1], "-s") == 0)
        {
            seconds = 3;
        }

        if (strcmp(argv[i-1], "--time") == 0 ||
            strcmp(argv[i - 1], "-t") == 0)
        {
            seconds = atoi(argv[i]);
        }

        if (strcmp(argv[i - 1], "--domain") == 0 ||
            strcmp(argv[i - 1], "-d") == 0)
        {
            domain = atoi(argv[i]);
        }
    }

    Sniffer snif;
    if (snif.init(domain))
    {
        snif.run(seconds);
    }

    return 0;
}

Sniffer::Sniffer() : 
    _participant(nullptr),
    _subscriber(nullptr),
    _topics(),
    _readers(),
    _datas(),
    _subscriberAttributes(),
    _readerQoS()
{
}

Sniffer::~Sniffer()
{
    _running = false;

    for (const auto& it : _topics)
    {
        if (_subscriber != nullptr)
        {
            _subscriber->delete_datareader(it.first);
        }
        _participant->delete_topic(it.second);
    }
    if (_subscriber != nullptr)
    {
        if (_participant != nullptr)
        {
            _participant->delete_subscriber(_subscriber);
        }
    }
    if (_participant != nullptr)
    {
        eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->delete_participant(_participant);
    }

    _datas.clear();
    _topics.clear();
    _readers.clear();
}

bool Sniffer::init(eprosima::fastdds::dds::DomainId_t domain)
{
    eprosima::fastdds::dds::DomainParticipantQos participantQoS;
    participantQoS.name("dds-sniffer");
    _participant = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->create_participant(domain, participantQoS, this);

    std::cout << "Creating a sniffer for domain " << domain << std::endl;

    return _participant != nullptr;
}

void Sniffer::run(uint32_t seconds)
{
    _running = true;

    if (seconds == 0)
    {
        std::cout << "\nPress enter to terminate\n" << std::endl;
        std::cin.ignore();
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    }
}

void Sniffer::on_data_available(eprosima::fastdds::dds::DataReader* reader)
{
    if (_running)
    {
        auto dataIter = _datas.find(reader);

        if (dataIter != _datas.end())
        {
            eprosima::fastrtps::types::DynamicData_ptr data = dataIter->second;
            eprosima::fastdds::dds::SampleInfo info;
            if (reader->take_next_sample(data.get(), &info) == ReturnCode_t::RETCODE_OK)
            {
                if (info.instance_state == eprosima::fastdds::dds::ALIVE_INSTANCE_STATE)
                {
                    eprosima::fastrtps::types::DynamicType_ptr type = _readers[reader];
                    std::cout << "Received data of type " << type->get_name() << std::endl;
                    eprosima::fastrtps::types::DynamicDataHelper::print(data);
                }
            }
        }
    }
}

void Sniffer::on_subscription_matched(eprosima::fastdds::dds::DataReader* reader,
                                      const eprosima::fastdds::dds::SubscriptionMatchedStatus& info)
{
    if (_running)
    {
        if (info.current_count_change == 1)
        {
            _matched = info.current_count; //MatchedStatus::current_count represents number of writers matched for a reader, total_count represents number of readers matched for a writer.
            std::cout << "A reader has been added to the domain" << std::endl;
        }
        else if (info.current_count_change == -1)
        {
            _matched = info.current_count; //MatchedStatus::current_count represents number of writers matched for a reader, total_count represents number of readers matched for a writer.
            std::cout << "A reader has been removed from the domain" << std::endl;
        }
        else
        {
            std::cout << "Invalid current_count_change value " << info.current_count_change << std::endl;
        }
    }
}

void Sniffer::on_type_discovery(eprosima::fastdds::dds::DomainParticipant*,
                                const eprosima::fastrtps::rtps::SampleIdentity&,
                                const eprosima::fastrtps::string_255& topic_name,
                                const eprosima::fastrtps::types::TypeIdentifier*,
                                const eprosima::fastrtps::types::TypeObject*,
                                eprosima::fastrtps::types::DynamicType_ptr dyn_type)
{
    if (_running)
    {
        eprosima::fastdds::dds::TypeSupport m_type(new eprosima::fastrtps::types::DynamicPubSubType(dyn_type));
        m_type.register_type(_participant);

        std::cout << "Discovered type: " << m_type->getName() << " from topic " << topic_name << std::endl;

        //Create a data reader for the topic
        if (_subscriber == nullptr)
        {
            _subscriber = _participant->create_subscriber(eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT, nullptr);

            if (_subscriber == nullptr)
            {
                return;
            }
        }

        eprosima::fastdds::dds::Topic* topic = _participant->create_topic(topic_name.c_str(), m_type->getName(),
                                                                          eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
        if (topic == nullptr)
        {
            return;
        }

        eprosima::fastdds::dds::DataReader* reader = _subscriber->create_datareader(topic, _readerQoS, this);

        _topics[reader] = topic;
        _readers[reader] = dyn_type;
        eprosima::fastrtps::types::DynamicData_ptr data(eprosima::fastrtps::types::DynamicDataFactory::get_instance()->create_data(dyn_type));
        _datas[reader] = data;
    }
}

void Sniffer::on_publisher_discovery(eprosima::fastdds::dds::DomainParticipant*,
                                     eprosima::fastrtps::rtps::WriterDiscoveryInfo&& info)
{
    if (_running)
    {
        switch (info.status)
        {
            case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
                std::cout << "DataWriter publishing topic '" << info.info.topicName()
                          << "' of type '" << info.info.typeName() << "' discovered" << std::endl;
                break;
            case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
                std::cout << "DataWriter publishing topic '" << info.info.topicName()
                          << "' of type '" << info.info.typeName() << "' left the domain." << std::endl;
                break;
        }
    }
}

void Sniffer::on_subscriber_discovery(eprosima::fastdds::dds::DomainParticipant*,
                                      eprosima::fastrtps::rtps::ReaderDiscoveryInfo&& info)
{
    if (_running)
    {
        switch (info.status)
        {
            case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::DISCOVERED_READER:
                /* Process the case when a new reader was found in the domain */
                std::cout << "DataReader reading topic '" << info.info.topicName()
                          << "' of type '" << info.info.typeName() << "' discovered" << std::endl;
                break;
            case eprosima::fastrtps::rtps::ReaderDiscoveryInfo::REMOVED_READER:
                /* Process the case when a reader was removed from the domain */
                std::cout << "DataReader reading topic '" << info.info.topicName()
                          << "' of type '" << info.info.typeName() << "' left the domain." << std::endl;
                break;
        }
    }
}

void Sniffer::on_participant_discovery(eprosima::fastdds::dds::DomainParticipant*,
                                       eprosima::fastrtps::rtps::ParticipantDiscoveryInfo&& info)
{
    if (_running)
    {
        switch (info.status)
        {
            case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DISCOVERED_PARTICIPANT:
                std::cout << "Participant '" << info.info.m_participantName << "' discovered" << std::endl;
                break;
            case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::REMOVED_PARTICIPANT:
            case eprosima::fastrtps::rtps::ParticipantDiscoveryInfo::DROPPED_PARTICIPANT:
                std::cout << "Participant " << info.info.m_participantName << "' left the domain." << std::endl;
                break;
        }
    }
}

void Sniffer::on_type_information_received(eprosima::fastdds::dds::DomainParticipant*,
                                           const eprosima::fastrtps::string_255 topic_name,
                                           const eprosima::fastrtps::string_255 type_name,
                                           const eprosima::fastrtps::types::TypeInformation& type_information)
{
    std::cout << "Topic '" << topic_name << "' with type '" << type_name << "' received" << std::endl;
}
