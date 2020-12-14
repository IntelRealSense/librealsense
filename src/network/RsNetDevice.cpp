// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#include <httplib.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "RsNetDevice.h"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

#include <zstd.h>
#include <zstd_errors.h>

#include <lz4.h>

#include <stdlib.h>

using namespace std::placeholders;

////

class RsMediaSubsession; // forward

class RsMediaSession : public MediaSession
{
public:
    static RsMediaSession* createNew(UsageEnvironment& env, char const* sdpDescription);

protected:
    RsMediaSession(UsageEnvironment& env) : MediaSession(env) {};
    virtual ~RsMediaSession() {};

    virtual MediaSubsession* createNewMediaSubsession();

    friend class RsMediaSubsessionIterator;
};

class RsMediaSubsessionIterator
{
public:
    RsMediaSubsessionIterator(RsMediaSession const& session) : fOurSession(session) { reset(); };
    virtual ~RsMediaSubsessionIterator() {};

    RsMediaSubsession* next();
    void reset();

private:
    RsMediaSession const& fOurSession;
    RsMediaSubsession* fNextPtr;
};

class RsMediaSubsession : public MediaSubsession
{
protected:
    friend class RsMediaSession;
    friend class RsMediaSubsessionIterator;

    RsMediaSubsession(RsMediaSession& parent) : MediaSubsession(parent) {};
    virtual ~RsMediaSubsession() {};

    virtual Boolean createSourceObjects(int useSpecialRTPoffset) {    
        if (strcmp(fCodecName, "RS") == 0) {
            fReadSource = fRTPSource = RsVideoRTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat, fRTPTimestampFrequency, "video/LZ4");
            return True;
        }
        return MediaSubsession::createSourceObjects(useSpecialRTPoffset);
    };

};

RsMediaSession* RsMediaSession::createNew(UsageEnvironment& env, char const* sdpDescription)
{ 
    RsMediaSession* newSession = new RsMediaSession(env);
    if(newSession != NULL) {
        if(!newSession->initializeWithSDP(sdpDescription)) {
            delete newSession;
            return NULL;
        }
    }
    return newSession;
}

MediaSubsession* RsMediaSession::createNewMediaSubsession() {
    return new RsMediaSubsession(*this); 
}

RsMediaSubsession* RsMediaSubsessionIterator::next() {
    RsMediaSubsession* result = fNextPtr;
    if (fNextPtr != NULL) fNextPtr = (RsMediaSubsession*)(fNextPtr->fNext);
    return result;
}

void RsMediaSubsessionIterator::reset() {
    fNextPtr = (RsMediaSubsession*)(fOurSession.fSubsessionsHead); 
}

////

void rs_net_sensor::doRTP() {
    std::stringstream ss;
    ss << std::setiosflags(std::ios::left) << std::setw(14) << m_name << ": RTP support thread started" << std::endl;
    std::cout << ss.str();

    TaskScheduler* scheduler = BasicTaskScheduler::createNew(/* 1000 */); // Check this later
    m_env = BasicUsageEnvironment::createNew(*scheduler);

    // Start the watch thread  
    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);

    // Start the scheduler
    m_env->taskScheduler().doEventLoop(&m_eventLoopWatchVariable);

    std::cout << m_name << " : RTP support thread exited" << std::endl;
}

void rs_net_sensor::doControl() {
    bool streaming = is_streaming();
    if (streaming != m_streaming) {
        // sensor state changed
        m_streaming = streaming;
        if (is_streaming()) {
            std::cout << "Sensor enabled\n";

            // Create RTSP client
            RTSPClient::responseBufferSize = 100000;
            m_rtspClient = RSRTSPClient::createNew(*m_env, m_mrl.c_str());
            if (m_rtspClient == NULL) {
                std::cout << "Failed to create a RTSP client for URL '" << m_mrl << "': " << m_env->getResultMsg() << std::endl;
                throw std::runtime_error("Cannot create RTSP client");
            }
            std::cout << "Connected to " << m_mrl << std::endl;

            // Prepare profiles list and allocate the queues
            m_streams.clear();
            // m_dev_flag = true;
            for (auto stream_profile : m_sw_sensor->get_active_streams()) {
                rs_net_stream* net_stream = new rs_net_stream(stream_profile);
                uint64_t key = slib::profile2key(net_stream->profile);
                m_streams[key] = net_stream;
            }
            
            // Start playing streams
            m_rtspClient->playStreams(m_streams);

            // Start SW device thread
            m_dev_flag = true;
            for (auto ks : m_streams) {
                rs_net_stream* net_stream = ks.second;
                net_stream->thread = std::thread( [&](){ doDevice(ks.first); });
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            }
            // m_dev = std::thread( [&](){ doDevice(); });
        } else {
            std::cout << "Sensor disabled\n";

            // Stop SW device thread
            m_dev_flag = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            for (auto ks : m_streams) {
                auto net_stream = ks.second;
                if (net_stream->thread.joinable()) net_stream->thread.join();
            }

            // disable running RTP sessions
            m_rtspClient->shutdownStream();
            m_rtspClient = NULL;

            // remove the queues and their content
            for (auto ks : m_streams) {
                auto net_stream = ks.second;
                while (!net_stream->queue->empty()) {
                    delete net_stream->queue->front();
                    net_stream->queue->pop();
                }
                // delete net_stream->queue;
                delete net_stream;
            }
            m_streams.clear();
        }
    }

    m_env->taskScheduler().scheduleDelayedTask(100000, doControl, this);
}

uint32_t chunks_allocated = 0;

void rs_net_sensor::doDevice(uint64_t key) {
    uint32_t fps_frame_count = 0;
    auto beginning = std::chrono::system_clock::now();

    uint32_t frame_size;
    rs_net_stream* net_stream = m_streams[key];
    if (net_stream->profile.is<rs2::video_stream_profile>()) {
        rs2::video_stream_profile vsp = net_stream->profile.as<rs2::video_stream_profile>();
        uint32_t bpp = (vsp.stream_type() == RS2_STREAM_INFRARED) ? 1 : 2; // IR:1 COLOR and DEPTH:2
        frame_size = vsp.width() * vsp.height() * bpp;
    } else if (net_stream->profile.is<rs2::motion_stream_profile>()) {
        frame_size = 32;
    } else throw std::runtime_error("Unknown profile on SW device support thread start.");

    // rs2_video_stream s = slib::key2stream(key);
    std::cout << m_name << "/" << rs2_stream_to_string(net_stream->profile.stream_type()) << "\t: SW device support thread started" << std::endl;

    int frame_count = 0; 
    bool prev_sensor_state = false;

    while (m_dev_flag) {
        if (net_stream->queue->empty()) continue;

        auto start = std::chrono::system_clock::now();

        uint32_t size = 0;
        uint32_t offset = 0;
        uint32_t total_size = 0;

        while (offset < frame_size) {
            uint8_t* data = 0;
            do {
                if (!m_dev_flag) goto out;
                data = net_stream->queue->front();
            } while (data == 0);
            chunk_header_t* ch = (chunk_header_t*)data;

            if (ch->offset < offset) break;

            total_size += ch->size;
            offset = ch->offset;
            int ret = 0;
            switch (ch->status & 3) {
            case 0:
                ret = ch->size - CHUNK_HLEN;
                memcpy((void*)(net_stream->m_frame_raw + offset), (void*)(data + CHUNK_HLEN), ret);
                break;
            case 1: 
                ret = ZSTD_decompress((void*)(net_stream->m_frame_raw + offset), CHUNK_SIZE, (void*)(data + CHUNK_HLEN), ch->size - CHUNK_HLEN); 
                break;
            case 2: 
                ret = LZ4_decompress_safe((const char*)(data + CHUNK_HLEN), (char*)(net_stream->m_frame_raw + offset), ch->size - CHUNK_HLEN, CHUNK_SIZE);
                ret = ch->size - CHUNK_HLEN;
                break;
            case 3:
                std::cout << "JPEG not implemented yet" << std::endl;
                break;
            }
            size += ret;
            // offset += CHUNK_SIZE;
            offset += ret;

            net_stream->queue->pop();
            delete [] data;
        } 

        uint8_t* frame_raw = new uint8_t[frame_size];
        memcpy(frame_raw, net_stream->m_frame_raw, frame_size);

        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::chrono::duration<double> total_time = end - beginning;
        fps_frame_count++;
        double fps;
        if (total_time.count() > 0) fps = (double)fps_frame_count / (double)total_time.count();
        else fps = 0;

        std::stringstream ss_name;
        ss_name << "Frame '" << std::setiosflags(std::ios::left);
        ss_name << std::setw(13) << m_name << " / " << rs2_stream_to_string(net_stream->profile.stream_type());
        if (net_stream->profile.stream_index()) ss_name << " " << net_stream->profile.stream_index();
        ss_name << "'";

        std::stringstream ss;
        ss << std::setiosflags(std::ios::left) << std::setw(35) << ss_name.str();
        ss << std::setiosflags(std::ios::right) << std::setiosflags(std::ios::fixed) << std::setprecision(2); 
        ss << " decompression time " << std::setw(7) << elapsed.count() * 1000 << "ms, ";
        ss << "size " << std::setw(7) << total_size << " => " << std::setw(7) << size << ", ";
        ss << "FPS: " << std::setw(7) << fps << std::endl;

        std::cout << ss.str();
        
        if (total_time > std::chrono::seconds(1)) {
            beginning = std::chrono::system_clock::now();
            fps_frame_count = 0;
        }

        // std::cout << "Chunks: " << chunks_allocated << "\n"; 

        // send it into device
        if (net_stream->profile.is<rs2::video_stream_profile>()) {
            rs2::video_stream_profile vsp = net_stream->profile.as<rs2::video_stream_profile>();
            int bpp = (vsp.stream_type() == RS2_STREAM_INFRARED) ? 1 : 2; // IR:1 COLOR and DEPTH:2
            m_sw_sensor->on_video_frame(
                { 
                    (void*)frame_raw, 
                    [] (void* f) { delete [] (uint8_t*)f; }, 
                    vsp.width() * bpp,
                    bpp,                                                  
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    vsp
                }
            );
        } else if (net_stream->profile.is<rs2::motion_stream_profile>()){
            rs2::motion_stream_profile msp = net_stream->profile.as<rs2::motion_stream_profile>();
            m_sw_sensor->on_motion_frame(
                { 
                    (void*)frame_raw, 
                    [] (void* f) { delete [] (uint8_t*)f; }, 
                    (double)time(NULL), 
                    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, 
                    ++frame_count, 
                    msp
                }
            );
        } else throw std::runtime_error("Unknown profile on frame departure.");
    }

out:
    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10)); 
    std::cout << m_name << "/" << rs2_stream_to_string(net_stream->profile.stream_type()) << "\t: SW device support thread exited" << std::endl;
}

rs_net_device::rs_net_device(rs2::software_device sw_device, std::string ip_address)
    : m_device(sw_device)
{
    // parse the parameters and set address and port
    int colon = ip_address.find(":");
    m_ip_address = ip_address.substr(0, colon); // 10.10.10.10:8554 => 10.10.10.10
    m_ip_port = 8554; // default RTSP port
    if (colon != -1)
        try
        {
            m_ip_port = std::stoi(ip_address.substr(colon + 1)); // 10.10.10.10:8554 => 8554
        }
        catch(...) // ignore wrong port and use default
        {
            std::cout << "WARNING: invalid port specified, using default";
        }

    // Obtain number of sensors and their names via HTTP and construct software device. 
    httplib::Client client(m_ip_address, 8080);
    auto res = client.Get("/query");

    if (res->status == 200) {
        // parse the response in form:
        // <sensor_name>|<sensor_mrl>|<sensor_profile1>|<sensor_profile2>|...|<sensor_profileN>

        std::string query = res->body;
        while (!query.empty()) {
            // get the sensor line
            uint32_t line_pos = query.find("\r\n");
            std::string sensor = query.substr(0, line_pos) + "|";
            query.erase(0, line_pos + 2);

            // get the sensor name
            uint32_t pos = sensor.find("|");
            std::string sensor_name = sensor.substr(0, pos);
            sensor.erase(0, pos + 1);

            // TODO: move the following code to the NetSensor's constructor
            NetSensor netsensor(new rs_net_sensor(m_device, sensor_name));

            // get the sensor MRL
            pos = sensor.find("|");
            netsensor->set_mrl(sensor.substr(0, pos));
            sensor.erase(0, pos + 1);

            while (!sensor.empty()) {
                pos = sensor.find("|");
                uint64_t key = std::stoull(sensor.substr(0, pos).c_str());
                sensor.erase(0, pos + 1);
                netsensor->add_profile(key);
            }

            sensors.emplace_back(netsensor);
        }
    }

    std::cout << "Software device is ready" << std::endl;

    intrinsics = { 640, 480, (float)640 / 2, (float)480 / 2, (float)640 / 2, (float)480 / 2, RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

    for (auto netsensor : sensors) netsensor->start();
}

void RSRTSPClient::shutdownStream() {
    // First, check whether any subsessions have still to be closed:
    if (m_session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*m_session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;
                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }
                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            sendTeardownCommand(*m_session, NULL);
        }

        m_session->close(m_session);
        m_session = NULL;
    }

    std::cout << "Closing the stream.\n";
    Medium::close(this);
}

// static callback
void RSRTSPClient::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterDESCRIBE(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterDESCRIBE(int resultCode, char* resultString) {
    if (resultCode != 0) {
        std::cout << "Failed to get a SDP description: " << resultString << "\n";
        delete[] resultString;
        throw std::runtime_error("Failed to get a SDP description");
    } else {
        m_sdp = std::string(resultString);
        delete[] resultString; // because we don't need it anymore
    }

    prepareSession();
    playSession();
}

void RSRTSPClient::prepareSession() {
    UsageEnvironment& env = envir(); // alias

    // Create a media session object from this SDP description:
    // std::cout << "SDP description:\n" << m_sdp << "\n";
    m_session = RsMediaSession::createNew(env, m_sdp.c_str());

    if (m_session == NULL) {
        std::cout << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
        throw std::runtime_error("Malformed server response");
    } else if (!m_session->hasSubsessions()) {
        std::cout << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
        throw std::runtime_error("No profiles found");
    } else {
        std::cout << "Session created " << m_session->name() << "/" << m_session->sessionDescription() << "/" << m_session->controlPath() << "\n";
        return;
    }

    // An unrecoverable error occurred with this stream.
    shutdownStream();
}

void RSRTSPClient::playSession() {
    // setup streams first
    if (m_streams_it == NULL) m_streams_it = new std::map<uint64_t, rs_net_stream*>::iterator(m_streams.begin());

    while (*m_streams_it != m_streams.end()) {
        rs2::stream_profile profile = (*m_streams_it)->second->profile;
        uint64_t profile_key = slib::profile2key(profile);

        bool profile_found = false;
        MediaSubsessionIterator it(*m_session);

        std::cout << "Looking  for " << profile_key << "\t" << slib::print_profile(profile) << std::endl;
        while (m_subsession = it.next()) {
            uint64_t subsession_key = std::stoull(m_subsession->attrVal_str("key"));
            rs2_video_stream vs = slib::key2stream(subsession_key);
            std::cout << "Checking for " << subsession_key << "\t" << slib::print_stream(&vs) << std::endl;
            if (profile_key == subsession_key) {
                std::cout << "Profile match for " << m_subsession->controlPath() << std::endl;
                profile_found = true;

                int useSpecialRTPoffset = -1; // for supported codecs
                if (strcmp(m_subsession->codecName(), "RS") == 0) {
                    useSpecialRTPoffset = 0;
                }

                if (!m_subsession->initiate(useSpecialRTPoffset)) {
                    std::cout << "Failed to initiate the \"" << m_subsession->controlPath() << "\" subsession: " << envir().getResultMsg() << "\n";
                } else {
                    std::cout << "Initiated the '" << std::setw(10) << m_subsession->controlPath() << "' " 
                                                << m_subsession->mediumName()   << "/" 
                                                << m_subsession->protocolName() << "/" 
                                                << m_subsession->videoWidth() << "x" << m_subsession->videoHeight() << "x" << m_subsession->videoFPS() << " subsession (";
                    if (m_subsession->rtcpIsMuxed()) {
                        std::cout << "client port " << m_subsession->clientPortNum();
                    } else {
                        std::cout << "client ports " << m_subsession->clientPortNum() << "-" << m_subsession->clientPortNum() + 1;
                    }
                    std::cout << ") [" << m_subsession->readSource()->name() << " : " << m_subsession->readSource()->MIMEtype() << "]\n";

                    // Continue setting up this subsession, by sending a RTSP "SETUP" command:
                    sendSetupCommand(*m_subsession, RSRTSPClient::continueAfterSETUP);
                    return;
                }
            }
        }
        if (!profile_found) throw std::runtime_error("Cannot match a profile");
    }

    delete m_streams_it;
    m_streams_it = NULL;

    sendPlayCommand(*m_session, continueAfterPLAY);
}

// static callback
void RSRTSPClient::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterSETUP(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterSETUP(int resultCode, char* resultString)
{
    UsageEnvironment& env = envir(); // alias
    
    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)
    m_subsession->sink = RSSink::createNew(env, *m_subsession, url(), (*m_streams_it)->second->queue, m_subsession->videoWidth() * m_subsession->videoHeight() * m_subsession->videoFPS());

    // do not wait for the out of order packets
    // m_scs.subsession->rtpSource()->setPacketReorderingThresholdTime(0); 

    if (m_subsession->sink == NULL) {
        env << "Failed to create a data sink for the '" << m_subsession->controlPath() << "' subsession: " << env.getResultMsg() << "\n";
    } else {
        env << "Created a data sink for the \"" << m_subsession->controlPath() << "\" subsession\n";
        m_subsession->miscPtr = this; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        m_subsession->sink->startPlaying(*(m_subsession->readSource()), subsessionAfterPlaying, m_subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (m_subsession->rtcpInstance() != NULL) {
            m_subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, m_subsession);
        }
    }

    // setup the remainded streams
    (*m_streams_it)++;
    playSession();
}

void RSRTSPClient::playStreams(std::map<uint64_t, rs_net_stream*> streams) {
    m_streams = streams;
    sendDescribeCommand(RSRTSPClient::continueAfterDESCRIBE);
}

// static callback
void RSRTSPClient::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterPLAY(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterPLAY(int resultCode, char* resultString) {
    UsageEnvironment& env = envir(); // alias

    if (resultCode != 0) {
        env << "Failed to start playing session: " << resultString << "\n";

        // An unrecoverable error occurred with this stream.
        shutdownStream();
    } else {
        env << "Started playing session...\n";
    }

    delete[] resultString;
}

void RSRTSPClient::subsessionAfterPlaying(void* clientData)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RSRTSPClient* rtspClient = (RSRTSPClient*)(subsession->miscPtr);

    std::cout << "Closing " << subsession->controlPath() << "session" << std::endl;

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while((subsession = iter.next()) != NULL)
    {
        if(subsession->sink != NULL)
            return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    rtspClient->shutdownStream();
}

void RSRTSPClient::subsessionByeHandler(void* clientData, char const* reason)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RSRTSPClient* rtspClient = (RSRTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << "Received RTCP \"BYE\"";
    if(reason != NULL)
    {
        env << " (reason:\"" << reason << "\")";
        delete[](char*) reason;
    }
    env << " on \"" << subsession->controlPath() << "\" subsession\n";

    // Now act as if the subsession had closed:
    rtspClient->subsessionAfterPlaying(subsession);
}

// Implementation of "RSSink":
#define RS_SINK_RECEIVE_BUFFER_SIZE (CHUNK_SIZE + CHUNK_HLEN)

RSSink* RSSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId, SafeQueue* q, uint32_t threshold)
{
    return new RSSink(env, subsession, streamId, q, threshold);
}

RSSink::RSSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId, SafeQueue* q, uint32_t threshold)
    : MediaSink(env), fSubsession(subsession), m_threshold(threshold), m_frames(q)
{
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[RS_SINK_RECEIVE_BUFFER_SIZE];
    chunks_allocated++;
}

RSSink::~RSSink()
{
    if (fReceiveBuffer) delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void RSSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
    RSSink* sink = (RSSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void RSSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
    // introducing scope for the lock_guard
    { 
        // std::lock_guard<std::mutex> lck (m_frames_mutex);
        // if (m_threshold / CHUNK_SIZE < m_frames.size()) {
        //     std::cout << "Dropping " << m_frames.size() << " chunks" << std::endl;
        //     chunks_allocated -= m_frames.size();
        //     while (!m_frames.empty()) {
        //         delete [] m_frames.front();
        //         m_frames.pop();
        //     }
        // }
        m_frames->push(fReceiveBuffer);
        fReceiveBuffer = new u_int8_t[RS_SINK_RECEIVE_BUFFER_SIZE];
        chunks_allocated++;
    }
 
    // Then continue, to request the next frame of data:
    continuePlaying();
}

void RSSink::getNextFrame(RSSink* sink) {
    sink->continuePlaying();
};

Boolean RSSink::continuePlaying() {
    if(fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, RS_SINK_RECEIVE_BUFFER_SIZE, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
