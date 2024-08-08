// Copyright (c) 2009, Willow Garage, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Willow Garage, Inc. nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "rosbag/bag.h"
#include "rosbag/message_instance.h"
#include "rosbag/query.h"
#include "rosbag/view.h"

#if defined(_MSC_VER)
  #include <stdint.h> // only on v2010 and later -> is this enough for msvc and linux?
#else
  #include <inttypes.h>
#endif
#include <signal.h>
#include <assert.h>
#include <iomanip>
#include <map>
#include <tuple>
#include <tuple>

#include "console_bridge/console.h"
#include <memory.h>

using std::map;
using std::string;
using std::vector;
using std::multiset;
using std::shared_ptr;
using rs2rosinternal::M_string;
using rs2rosinternal::Time;

namespace rosbag {

Bag::Bag() :
    mode_(bagmode::Write),
    version_(0),
    compression_(compression::Uncompressed),
    chunk_threshold_(768 * 1024),  // 768KB chunks
    bag_revision_(0),
    file_size_(0),
    file_header_pos_(0),
    index_data_pos_(0),
    connection_count_(0),
    chunk_count_(0),
    chunk_open_(false),
    curr_chunk_data_pos_(0),
    current_buffer_(0),
    decompressed_chunk_(0)
{
}

Bag::Bag(string const& filename, uint32_t mode) :
    compression_(compression::Uncompressed),
    chunk_threshold_(768 * 1024),  // 768KB chunks
    bag_revision_(0),
    file_size_(0),
    file_header_pos_(0),
    index_data_pos_(0),
    connection_count_(0),
    chunk_count_(0),
    chunk_open_(false),
    curr_chunk_data_pos_(0),
    current_buffer_(0),
    decompressed_chunk_(0)
{
    open(filename, mode);
}

Bag::~Bag() {
    close();
}

void Bag::open(string const& filename, uint32_t mode) {
    mode_ = (BagMode) mode;

    if (mode_ & bagmode::Append)
        openAppend(filename);
    else if (mode_ & bagmode::Write)
        openWrite(filename);
    else if (mode_ & bagmode::Read)
        openRead(filename);
    else
        throw BagException( "Unknown mode: " + std::to_string( (int)mode ) );

    // Determine file size
    uint64_t offset = file_.getOffset();
    seek(0, std::ios::end);
    file_size_ = file_.getOffset();
    seek(offset);
}

void Bag::openRead(string const& filename) {
    file_.openRead(filename);

    readVersion();

    switch (version_) {
    case 102: startReadingVersion102(); break;
    case 200: startReadingVersion200(); break;
    default:
        throw BagException( "Unsupported bag file version: " + std::to_string( getMajorVersion() ) + '.'
                            + std::to_string( getMinorVersion() ) );
    }
}

void Bag::openWrite(string const& filename) {
    file_.openWrite(filename);

    startWriting();
}

void Bag::openAppend(string const& filename) {
    file_.openReadWrite(filename);

    readVersion();

    if (version_ != 200)
        throw BagException( "Bag file version " + std::to_string( getMajorVersion() ) + '.'
                            + std::to_string( getMinorVersion() ) + " is unsupported for appending" );

    startReadingVersion200();

    // Truncate the file to chop off the index
    file_.truncate(index_data_pos_);
    index_data_pos_ = 0;

    // Rewrite the file header, clearing the index position (so we know if the index is invalid)
    seek(file_header_pos_);
    writeFileHeaderRecord();

    // Seek to the end of the file
    seek(0, std::ios::end);
}

void Bag::close() {
    if (!file_.isOpen())
        return;

    if (mode_ & bagmode::Write || mode_ & bagmode::Append)
        closeWrite();

    file_.close();

    topic_connection_ids_.clear();
    header_connection_ids_.clear();
    for (map<uint32_t, ConnectionInfo*>::iterator i = connections_.begin(); i != connections_.end(); i++)
        delete i->second;
    connections_.clear();
    chunks_.clear();
    connection_indexes_.clear();
    curr_chunk_connection_indexes_.clear();
}

void Bag::closeWrite() {
    stopWriting();
}

string   Bag::getFileName() const { return file_.getFileName(); }
BagMode  Bag::getMode()     const { return mode_;               }
uint64_t Bag::getSize()     const { return file_size_;          }

uint32_t Bag::getChunkThreshold() const { return chunk_threshold_; }

void Bag::setChunkThreshold(uint32_t chunk_threshold) {
    if (file_.isOpen() && chunk_open_)
        stopWritingChunk();

    chunk_threshold_ = chunk_threshold;
}

CompressionType Bag::getCompression() const { return compression_; }

std::tuple<std::string, uint64_t, uint64_t> Bag::getCompressionInfo() const
{
    std::map<std::string, uint64_t> compression_counts;
    std::map<std::string, uint64_t> compression_uncompressed;
    std::map<std::string, uint64_t> compression_compressed;
    auto compression = compression_;
    uint64_t uncompressed = 0;
    uint64_t compressed = 0;
    for( ChunkInfo const & curr_chunk_info : chunks_ )
    {
        seek(curr_chunk_info.pos);

        // Skip over the chunk data
        ChunkHeader chunk_header;
        readChunkHeader(chunk_header);
        seek(chunk_header.compressed_size, std::ios::cur);

        compression_counts[chunk_header.compression] += 1;
        compression_uncompressed[chunk_header.compression] += chunk_header.uncompressed_size;
        uncompressed += chunk_header.uncompressed_size;
        compression_compressed[chunk_header.compression] += chunk_header.compressed_size;
        compressed += chunk_header.compressed_size;
    }

    auto chunk_count = chunks_.size();
    uint64_t main_compression_count = 0;
    std::string main_compression;
    for (auto&& kvp : compression_counts)
    {
        if (kvp.second > main_compression_count)
        {
            main_compression = kvp.first;
            main_compression_count = kvp.second;
        }
    }
    //auto main_compression_str = compression_counts.begin()->first;
    //CompressionType main_compression = (main_compression_str == "Uncompressed") ? CompressionType::Uncompressed : (main_compression_str == "LZ4") ? CompressionType::LZ4 : CompressionType::BZ2;
    //auto main_compression_count = compression_counts.begin()->second;
    return std::make_tuple(main_compression, compressed, uncompressed);
}
void Bag::setCompression(CompressionType compression) {
    if (file_.isOpen() && chunk_open_)
        stopWritingChunk();

    if (!(compression == compression::Uncompressed ||
          compression == compression::BZ2 ||
          compression == compression::LZ4)) {
        throw BagException( "Unknown compression type: " + std::to_string( (int)compression ) );
    }

    compression_ = compression;
}

// Version

void Bag::writeVersion() {
    string version = string("#ROSBAG V") + VERSION + string("\n");

    CONSOLE_BRIDGE_logDebug("Writing VERSION [%llu]: %s", (unsigned long long) file_.getOffset(), version.c_str());

    version_ = 200;

    write(version);
}

void Bag::readVersion() {
    string version_line = file_.getline();

    file_header_pos_ = file_.getOffset();

    char logtypename[100];
    int version_major, version_minor;
#if defined(_MSC_VER)
    if (sscanf_s(version_line.c_str(), "#ROS%s V%d.%d", logtypename, static_cast<unsigned>(sizeof(logtypename)), &version_major, &version_minor) != 3)
#else
    if (sscanf(version_line.c_str(), "#ROS%s V%d.%d", logtypename, &version_major, &version_minor) != 3)
#endif
        throw BagIOException("Error reading version line");

    version_ = version_major * 100 + version_minor;

    CONSOLE_BRIDGE_logDebug("Read VERSION: version=%d", version_);
}

uint32_t Bag::getMajorVersion() const { return version_ / 100; }
uint32_t Bag::getMinorVersion() const { return version_ % 100; }

//

void Bag::startWriting() {
    writeVersion();
    file_header_pos_ = file_.getOffset();
    writeFileHeaderRecord();
}

void Bag::stopWriting() {
    if (chunk_open_)
        stopWritingChunk();

    seek(0, std::ios::end);

    index_data_pos_ = file_.getOffset();
    writeConnectionRecords();
    writeChunkInfoRecords();

    seek(file_header_pos_);
    writeFileHeaderRecord();
}

void Bag::startReadingVersion200() {
    // Read the file header record, which points to the end of the chunks
    readFileHeaderRecord();

    // Seek to the end of the chunks
    seek(index_data_pos_);

    // Read the connection records (one for each connection)
    for (uint32_t i = 0; i < connection_count_; i++)
        readConnectionRecord();

    // Read the chunk info records
    for (uint32_t i = 0; i < chunk_count_; i++)
        readChunkInfoRecord();

    // Read the connection indexes for each chunk
    for( ChunkInfo const & chunk_info : chunks_ )
    {
        curr_chunk_info_ = chunk_info;

        seek(curr_chunk_info_.pos);

        // Skip over the chunk data
        ChunkHeader chunk_header;
        readChunkHeader(chunk_header);
        seek(chunk_header.compressed_size, std::ios::cur);

        // Read the index records after the chunk
        for (unsigned int i = 0; i < chunk_info.connection_counts.size(); i++)
            readConnectionIndexRecord200();
    }

    // At this point we don't have a curr_chunk_info anymore so we reset it
    curr_chunk_info_ = ChunkInfo();
}

void Bag::startReadingVersion102() {
    try
    {
        // Read the file header record, which points to the start of the topic indexes
        readFileHeaderRecord();
    }
    catch (BagFormatException ex) {
        throw BagUnindexedException();
    }

    // Get the length of the file
    seek(0, std::ios::end);
    uint64_t filelength = file_.getOffset();

    // Seek to the beginning of the topic index records
    seek(index_data_pos_);

    // Read the topic index records, which point to the offsets of each message in the file
    while (file_.getOffset() < filelength)
        readTopicIndexRecord102();

    // Read the message definition records (which are the first entry in the topic indexes)
    for (map<uint32_t, multiset<IndexEntry> >::const_iterator i = connection_indexes_.begin(); i != connection_indexes_.end(); i++) {
        multiset<IndexEntry> const& index       = i->second;
        IndexEntry const&           first_entry = *index.begin();

        CONSOLE_BRIDGE_logDebug("Reading message definition for connection %d at %llu", i->first, (unsigned long long) first_entry.chunk_pos);

        seek(first_entry.chunk_pos);

        readMessageDefinitionRecord102();
    }
}

// File header record

void Bag::writeFileHeaderRecord() {
    connection_count_ = static_cast<uint32_t>(connections_.size());
    chunk_count_      = static_cast<uint32_t>(chunks_.size());

    CONSOLE_BRIDGE_logDebug("Writing FILE_HEADER [%llu]: index_pos=%llu connection_count=%d chunk_count=%d",
              (unsigned long long) file_.getOffset(), (unsigned long long) index_data_pos_, connection_count_, chunk_count_);

    // Write file header record
    M_string header;
    header[OP_FIELD_NAME]               = toHeaderString(&OP_FILE_HEADER);
    header[INDEX_POS_FIELD_NAME]        = toHeaderString(&index_data_pos_);
    header[CONNECTION_COUNT_FIELD_NAME] = toHeaderString(&connection_count_);
    header[CHUNK_COUNT_FIELD_NAME]      = toHeaderString(&chunk_count_);

    std::vector<uint8_t> header_buffer;
    uint32_t header_len;
    rs2rosinternal::Header::write(header, header_buffer, header_len);
    uint32_t data_len = 0;
    if (header_len < FILE_HEADER_LENGTH)
        data_len = FILE_HEADER_LENGTH - header_len;
    write((char*) &header_len, 4);
    write((char*) header_buffer.data(), header_len);
    write((char*) &data_len, 4);

    // Pad the file header record out
    if (data_len > 0) {
        string padding;
        padding.resize(data_len, ' ');
        write(padding);
    }
}

void Bag::readFileHeaderRecord() {
    rs2rosinternal::Header header;
    uint32_t data_size;
    if (!readHeader(header) || !readDataLength(data_size))
        throw BagFormatException("Error reading FILE_HEADER record");

    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_FILE_HEADER))
        throw BagFormatException("Expected FILE_HEADER op not found");

    // Read index position
    readField(fields, INDEX_POS_FIELD_NAME, true, (uint64_t*) &index_data_pos_);

    if (index_data_pos_ == 0)
        throw BagUnindexedException();

    // Read topic and chunks count
    if (version_ >= 200) {
        readField(fields, CONNECTION_COUNT_FIELD_NAME, true, &connection_count_);
        readField(fields, CHUNK_COUNT_FIELD_NAME,      true, &chunk_count_);
    }

    CONSOLE_BRIDGE_logDebug("Read FILE_HEADER: index_pos=%llu connection_count=%d chunk_count=%d",
              (unsigned long long) index_data_pos_, connection_count_, chunk_count_);

    // Skip the data section (just padding)
    seek(data_size, std::ios::cur);
}

uint32_t Bag::getChunkOffset() const {
    if (compression_ == compression::Uncompressed)
        return static_cast<uint32_t>(file_.getOffset() - curr_chunk_data_pos_);
    else
        return file_.getCompressedBytesIn();
}

void Bag::startWritingChunk(Time time) {
    // Initialize chunk info
    curr_chunk_info_.pos        = file_.getOffset();
    curr_chunk_info_.start_time = time;
    curr_chunk_info_.end_time   = time;

    // Write the chunk header, with a place-holder for the data sizes (we'll fill in when the chunk is finished)
    writeChunkHeader(compression_, 0, 0);

    // Turn on compressed writing
    file_.setWriteMode(compression_);

    // Record where the data section of this chunk started
    curr_chunk_data_pos_ = file_.getOffset();

    chunk_open_ = true;
}

void Bag::stopWritingChunk() {
    // Add this chunk to the index
    chunks_.push_back(curr_chunk_info_);

    // Get the uncompressed and compressed sizes
    uint32_t uncompressed_size = getChunkOffset();
    file_.setWriteMode(compression::Uncompressed);
    uint32_t compressed_size = static_cast<uint32_t>(file_.getOffset() - curr_chunk_data_pos_);

    // Rewrite the chunk header with the size of the chunk (remembering current offset)
    uint64_t end_of_chunk_pos = file_.getOffset();

    seek(curr_chunk_info_.pos);
    writeChunkHeader(compression_, compressed_size, uncompressed_size);

    // Write out the indexes and clear them
    seek(end_of_chunk_pos);
    writeIndexRecords();
    curr_chunk_connection_indexes_.clear();

    // Clear the connection counts
    curr_chunk_info_.connection_counts.clear();

    // Flag that we're starting a new chunk
    chunk_open_ = false;
}

void Bag::writeChunkHeader(CompressionType compression, uint32_t compressed_size, uint32_t uncompressed_size) {
    ChunkHeader chunk_header;
    switch (compression) {
    case compression::Uncompressed: chunk_header.compression = COMPRESSION_NONE; break;
    case compression::BZ2:          chunk_header.compression = COMPRESSION_BZ2;  break;
    case compression::LZ4:          chunk_header.compression = COMPRESSION_LZ4;
    //case compression::ZLIB:         chunk_header.compression = COMPRESSION_ZLIB; break;
    }
    chunk_header.compressed_size   = compressed_size;
    chunk_header.uncompressed_size = uncompressed_size;

    CONSOLE_BRIDGE_logDebug("Writing CHUNK [%llu]: compression=%s compressed=%d uncompressed=%d",
              (unsigned long long) file_.getOffset(), chunk_header.compression.c_str(), chunk_header.compressed_size, chunk_header.uncompressed_size);

    M_string header;
    header[OP_FIELD_NAME]          = toHeaderString(&OP_CHUNK);
    header[COMPRESSION_FIELD_NAME] = chunk_header.compression;
    header[SIZE_FIELD_NAME]        = toHeaderString(&chunk_header.uncompressed_size);
    writeHeader(header);

    writeDataLength(chunk_header.compressed_size);
}

void Bag::readChunkHeader(ChunkHeader& chunk_header) const {
    rs2rosinternal::Header header;
    if (!readHeader(header) || !readDataLength(chunk_header.compressed_size))
        throw BagFormatException("Error reading CHUNK record");

    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_CHUNK))
        throw BagFormatException("Expected CHUNK op not found");

    readField(fields, COMPRESSION_FIELD_NAME, true, chunk_header.compression);
    readField(fields, SIZE_FIELD_NAME,        true, &chunk_header.uncompressed_size);

    CONSOLE_BRIDGE_logDebug("Read CHUNK: compression=%s size=%d uncompressed=%d (%f)", chunk_header.compression.c_str(), chunk_header.compressed_size, chunk_header.uncompressed_size, 100 * ((double) chunk_header.compressed_size) / chunk_header.uncompressed_size);
}

// Index records

void Bag::writeIndexRecords() {
    for (map<uint32_t, multiset<IndexEntry> >::const_iterator i = curr_chunk_connection_indexes_.begin(); i != curr_chunk_connection_indexes_.end(); i++) {
        uint32_t                    connection_id = i->first;
        multiset<IndexEntry> const& index         = i->second;

        // Write the index record header
        uint32_t index_size = static_cast<uint32_t>(index.size());
        M_string header;
        header[OP_FIELD_NAME]         = toHeaderString(&OP_INDEX_DATA);
        header[CONNECTION_FIELD_NAME] = toHeaderString(&connection_id);
        header[VER_FIELD_NAME]        = toHeaderString(&INDEX_VERSION);
        header[COUNT_FIELD_NAME]      = toHeaderString(&index_size);
        writeHeader(header);

        writeDataLength(index_size * 12);

        CONSOLE_BRIDGE_logDebug("Writing INDEX_DATA: connection=%d ver=%d count=%d", connection_id, INDEX_VERSION, index_size);

        // Write the index record data (pairs of timestamp and position in file)
        for( IndexEntry const & e : index )
        {
            write((char*) &e.time.sec,  4);
            write((char*) &e.time.nsec, 4);
            write((char*) &e.offset,    4);

            CONSOLE_BRIDGE_logDebug("  - %d.%d: %d", e.time.sec, e.time.nsec, e.offset);
        }
    }
}

void Bag::readTopicIndexRecord102() {
    rs2rosinternal::Header header;
    uint32_t data_size;
    if (!readHeader(header) || !readDataLength(data_size))
        throw BagFormatException("Error reading INDEX_DATA header");
    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_INDEX_DATA))
        throw BagFormatException("Expected INDEX_DATA record");

    uint32_t index_version;
    string topic;
    uint32_t count = 0;
    readField(fields, VER_FIELD_NAME,   true, &index_version);
    readField(fields, TOPIC_FIELD_NAME, true, topic);
    readField(fields, COUNT_FIELD_NAME, true, &count);

    CONSOLE_BRIDGE_logDebug("Read INDEX_DATA: ver=%d topic=%s count=%d", index_version, topic.c_str(), count);

    if (index_version != 0)
        throw BagFormatException( "Unsupported INDEX_DATA version: " + std::to_string( index_version ) );

    uint32_t connection_id;
    map<string, uint32_t>::const_iterator topic_conn_id_iter = topic_connection_ids_.find(topic);
    if (topic_conn_id_iter == topic_connection_ids_.end()) {
        connection_id = static_cast<uint32_t>(connections_.size());

        CONSOLE_BRIDGE_logDebug("Creating connection: id=%d topic=%s", connection_id, topic.c_str());
        ConnectionInfo* connection_info = new ConnectionInfo();
        connection_info->id       = connection_id;
        connection_info->topic    = topic;
        connections_[connection_id] = connection_info;

        topic_connection_ids_[topic] = connection_id;
    }
    else
        connection_id = topic_conn_id_iter->second;

    multiset<IndexEntry>& connection_index = connection_indexes_[connection_id];

    for (uint32_t i = 0; i < count; i++) {
        IndexEntry index_entry;
        uint32_t sec;
        uint32_t nsec;
        read((char*) &sec,                   4);
        read((char*) &nsec,                  4);
        read((char*) &index_entry.chunk_pos, 8);   //<! store position of the message in the chunk_pos field as it's 64 bits
        index_entry.time = Time(sec, nsec);
        index_entry.offset = 0;

        CONSOLE_BRIDGE_logDebug("  - %d.%d: %llu", sec, nsec, (unsigned long long) index_entry.chunk_pos);

        if (index_entry.time < rs2rosinternal::TIME_MIN || index_entry.time > rs2rosinternal::TIME_MAX)
        {
          CONSOLE_BRIDGE_logError("Index entry for topic %s contains invalid time.", topic.c_str());
        } else
        {
          connection_index.insert(connection_index.end(), index_entry);
        }
    }
}

void Bag::readConnectionIndexRecord200() {
    rs2rosinternal::Header header;
    uint32_t data_size;
    if (!readHeader(header) || !readDataLength(data_size))
        throw BagFormatException("Error reading INDEX_DATA header");
    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_INDEX_DATA))
        throw BagFormatException("Expected INDEX_DATA record");

    uint32_t index_version;
    uint32_t connection_id;
    uint32_t count = 0;
    readField(fields, VER_FIELD_NAME,        true, &index_version);
    readField(fields, CONNECTION_FIELD_NAME, true, &connection_id);
    readField(fields, COUNT_FIELD_NAME,      true, &count);

    CONSOLE_BRIDGE_logDebug("Read INDEX_DATA: ver=%d connection=%d count=%d", index_version, connection_id, count);

    if (index_version != 1)
        throw BagFormatException( "Unsupported INDEX_DATA version: " + std::to_string( index_version ) );

    uint64_t chunk_pos = curr_chunk_info_.pos;

    multiset<IndexEntry>& connection_index = connection_indexes_[connection_id];

    for (uint32_t i = 0; i < count; i++) {
        IndexEntry index_entry;
        index_entry.chunk_pos = chunk_pos;
        uint32_t sec;
        uint32_t nsec;
        read((char*) &sec,                4);
        read((char*) &nsec,               4);
        read((char*) &index_entry.offset, 4);
        index_entry.time = Time(sec, nsec);

        CONSOLE_BRIDGE_logDebug("  - %d.%d: %llu+%d", sec, nsec, (unsigned long long) index_entry.chunk_pos, index_entry.offset);

        if (index_entry.time < rs2rosinternal::TIME_MIN || index_entry.time > rs2rosinternal::TIME_MAX)
        {
            CONSOLE_BRIDGE_logError("Index entry for topic %s contains invalid time.  This message will not be loaded.", connections_[connection_id]->topic.c_str());
        } else
        {
          connection_index.insert(connection_index.end(), index_entry);
        }
    }
}

// Connection records

void Bag::writeConnectionRecords() {
    for (map<uint32_t, ConnectionInfo*>::const_iterator i = connections_.begin(); i != connections_.end(); i++) {
        ConnectionInfo const* connection_info = i->second;
        writeConnectionRecord(connection_info);
    }
}

void Bag::writeConnectionRecord(ConnectionInfo const* connection_info) {
    CONSOLE_BRIDGE_logDebug("Writing CONNECTION [%llu:%d]: topic=%s id=%d",
              (unsigned long long) file_.getOffset(), getChunkOffset(), connection_info->topic.c_str(), connection_info->id);

    M_string header;
    header[OP_FIELD_NAME]         = toHeaderString(&OP_CONNECTION);
    header[TOPIC_FIELD_NAME]      = connection_info->topic;
    header[CONNECTION_FIELD_NAME] = toHeaderString(&connection_info->id);
    writeHeader(header);

    writeHeader(*connection_info->header);
}

void Bag::appendConnectionRecordToBuffer(Buffer& buf, ConnectionInfo const* connection_info) {
    M_string header;
    header[OP_FIELD_NAME]         = toHeaderString(&OP_CONNECTION);
    header[TOPIC_FIELD_NAME]      = connection_info->topic;
    header[CONNECTION_FIELD_NAME] = toHeaderString(&connection_info->id);
    appendHeaderToBuffer(buf, header);

    appendHeaderToBuffer(buf, *connection_info->header);
}

void Bag::readConnectionRecord() {
    rs2rosinternal::Header header;
    if (!readHeader(header))
        throw BagFormatException("Error reading CONNECTION header");
    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_CONNECTION))
        throw BagFormatException("Expected CONNECTION op not found");

    uint32_t id;
    readField(fields, CONNECTION_FIELD_NAME, true, &id);
    string topic;
    readField(fields, TOPIC_FIELD_NAME,      true, topic);

    rs2rosinternal::Header connection_header;
    if (!readHeader(connection_header))
        throw BagFormatException("Error reading connection header");

    // If this is a new connection, update connections
    map<uint32_t, ConnectionInfo*>::iterator key = connections_.find(id);
    if (key == connections_.end()) {
        ConnectionInfo* connection_info = new ConnectionInfo();
        connection_info->id       = id;
        connection_info->topic    = topic;
        connection_info->header = std::make_shared<M_string>();
        for (M_string::const_iterator i = connection_header.getValues()->begin(); i != connection_header.getValues()->end(); i++)
            (*connection_info->header)[i->first] = i->second;
        connection_info->msg_def  = (*connection_info->header)["message_definition"];
        connection_info->datatype = (*connection_info->header)["type"];
        connection_info->md5sum   = (*connection_info->header)["md5sum"];
        connections_[id] = connection_info;

        CONSOLE_BRIDGE_logDebug("Read CONNECTION: topic=%s id=%d", topic.c_str(), id);
    }
}

void Bag::readMessageDefinitionRecord102() {
    rs2rosinternal::Header header;
    uint32_t data_size;
    if (!readHeader(header) || !readDataLength(data_size))
        throw BagFormatException("Error reading message definition header");
    M_string& fields = *header.getValues();

    if (!isOp(fields, OP_MSG_DEF))
        throw BagFormatException("Expected MSG_DEF op not found");

    string topic, md5sum, datatype, message_definition;
    readField(fields, TOPIC_FIELD_NAME,               true, topic);
    readField(fields, MD5_FIELD_NAME,   32,       32, true, md5sum);
    readField(fields, TYPE_FIELD_NAME,                true, datatype);
    readField(fields, DEF_FIELD_NAME,    0, UINT_MAX, true, message_definition);

    ConnectionInfo* connection_info;

    map<string, uint32_t>::const_iterator topic_conn_id_iter = topic_connection_ids_.find(topic);
    if (topic_conn_id_iter == topic_connection_ids_.end()) {
        uint32_t id = static_cast<uint32_t>(connections_.size());

        CONSOLE_BRIDGE_logDebug("Creating connection: topic=%s md5sum=%s datatype=%s", topic.c_str(), md5sum.c_str(), datatype.c_str());
        connection_info = new ConnectionInfo();
        connection_info->id       = id;
        connection_info->topic    = topic;

        connections_[id] = connection_info;
        topic_connection_ids_[topic] = id;
    }
    else
        connection_info = connections_[topic_conn_id_iter->second];

    connection_info->msg_def  = message_definition;
    connection_info->datatype = datatype;
    connection_info->md5sum   = md5sum;
    connection_info->header = std::make_shared<rs2rosinternal::M_string>();
    (*connection_info->header)["type"]               = connection_info->datatype;
    (*connection_info->header)["md5sum"]             = connection_info->md5sum;
    (*connection_info->header)["message_definition"] = connection_info->msg_def;

    CONSOLE_BRIDGE_logDebug("Read MSG_DEF: topic=%s md5sum=%s datatype=%s", topic.c_str(), md5sum.c_str(), datatype.c_str());
}

void Bag::decompressChunk(uint64_t chunk_pos) const {
    if (curr_chunk_info_.pos == chunk_pos) {
        current_buffer_ = &outgoing_chunk_buffer_;
        return;
    }

    current_buffer_ = &decompress_buffer_;

    if (decompressed_chunk_ == chunk_pos)
        return;

    // Seek to the start of the chunk
    seek(chunk_pos);

    // Read the chunk header
    ChunkHeader chunk_header;
    readChunkHeader(chunk_header);

    // Read and decompress the chunk.  These assume we are at the right place in the stream already
    if (chunk_header.compression == COMPRESSION_NONE)
        decompressRawChunk(chunk_header);
    else if (chunk_header.compression == COMPRESSION_BZ2)
        decompressBz2Chunk(chunk_header);
    else if (chunk_header.compression == COMPRESSION_LZ4)
        decompressLz4Chunk(chunk_header);
    else
        throw BagFormatException("Unknown compression: " + chunk_header.compression);

    decompressed_chunk_ = chunk_pos;
}

void Bag::readMessageDataRecord102(uint64_t offset, rs2rosinternal::Header& header) const {
    CONSOLE_BRIDGE_logDebug("readMessageDataRecord: offset=%llu", (unsigned long long) offset);

    seek(offset);

    uint32_t data_size;
    uint8_t op;
    do {
        if (!readHeader(header) || !readDataLength(data_size))
            throw BagFormatException("Error reading header");

        readField(*header.getValues(), OP_FIELD_NAME, true, &op);
    }
    while (op == OP_MSG_DEF);

    if (op != OP_MSG_DATA)
        throw BagFormatException( "Expected MSG_DATA op, got " + std::to_string( op ) );

    record_buffer_.setSize(data_size);
    file_.read((char*) record_buffer_.getData(), data_size);
}

// Reading this into a buffer isn't completely necessary, but we do it anyways for now
void Bag::decompressRawChunk(ChunkHeader const& chunk_header) const {
    assert(chunk_header.compression == COMPRESSION_NONE);
    assert(chunk_header.compressed_size == chunk_header.uncompressed_size);

    CONSOLE_BRIDGE_logDebug("compressed_size: %d uncompressed_size: %d", chunk_header.compressed_size, chunk_header.uncompressed_size);

    decompress_buffer_.setSize(chunk_header.compressed_size);
    file_.read((char*) decompress_buffer_.getData(), chunk_header.compressed_size);

    // todo check read was successful
}

void Bag::decompressBz2Chunk(ChunkHeader const& chunk_header) const {
    assert(chunk_header.compression == COMPRESSION_BZ2);

    CompressionType compression = compression::BZ2;

    CONSOLE_BRIDGE_logDebug("compressed_size: %d uncompressed_size: %d", chunk_header.compressed_size, chunk_header.uncompressed_size);

    chunk_buffer_.setSize(chunk_header.compressed_size);
    file_.read((char*) chunk_buffer_.getData(), chunk_header.compressed_size);

    decompress_buffer_.setSize(chunk_header.uncompressed_size);
    file_.decompress(compression, decompress_buffer_.getData(), decompress_buffer_.getSize(), chunk_buffer_.getData(), chunk_buffer_.getSize());

    // todo check read was successful
}

void Bag::decompressLz4Chunk(ChunkHeader const& chunk_header) const {
    assert(chunk_header.compression == COMPRESSION_LZ4);

    CompressionType compression = compression::LZ4;

    CONSOLE_BRIDGE_logDebug("lz4 compressed_size: %d uncompressed_size: %d",
             chunk_header.compressed_size, chunk_header.uncompressed_size);

    chunk_buffer_.setSize(chunk_header.compressed_size);
    file_.read((char*) chunk_buffer_.getData(), chunk_header.compressed_size);

    decompress_buffer_.setSize(chunk_header.uncompressed_size);
    file_.decompress(compression, decompress_buffer_.getData(), decompress_buffer_.getSize(), chunk_buffer_.getData(), chunk_buffer_.getSize());

    // todo check read was successful
}

rs2rosinternal::Header Bag::readMessageDataHeader(IndexEntry const& index_entry) {
    rs2rosinternal::Header header;
    uint32_t data_size;
    uint32_t bytes_read;
    switch (version_)
    {
    case 200:
        decompressChunk(index_entry.chunk_pos);
        readMessageDataHeaderFromBuffer(*current_buffer_, index_entry.offset, header, data_size, bytes_read);
        return header;
    case 102:
        readMessageDataRecord102(index_entry.chunk_pos, header);
        return header;
    default:
        throw BagFormatException( "Unhandled version: " + std::to_string( version_ ) );
    }
}

// NOTE: this loads the header, which is unnecessary
uint32_t Bag::readMessageDataSize(IndexEntry const& index_entry) const {
    rs2rosinternal::Header header;
    uint32_t data_size;
    uint32_t bytes_read;
    switch (version_)
    {
    case 200:
        decompressChunk(index_entry.chunk_pos);
        readMessageDataHeaderFromBuffer(*current_buffer_, index_entry.offset, header, data_size, bytes_read);
        return data_size;
    case 102:
        readMessageDataRecord102(index_entry.chunk_pos, header);
        return record_buffer_.getSize();
    default:
        throw BagFormatException( "Unhandled version: " + std::to_string( version_ ) );
    }
}

void Bag::writeChunkInfoRecords() {
    for( ChunkInfo const & chunk_info : chunks_ )
    {
        // Write the chunk info header
        M_string header;
        uint32_t chunk_connection_count = static_cast<uint32_t>(chunk_info.connection_counts.size());
        header[OP_FIELD_NAME]         = toHeaderString(&OP_CHUNK_INFO);
        header[VER_FIELD_NAME]        = toHeaderString(&CHUNK_INFO_VERSION);
        header[CHUNK_POS_FIELD_NAME]  = toHeaderString(&chunk_info.pos);
        header[START_TIME_FIELD_NAME] = toHeaderString(&chunk_info.start_time);
        header[END_TIME_FIELD_NAME]   = toHeaderString(&chunk_info.end_time);
        header[COUNT_FIELD_NAME]      = toHeaderString(&chunk_connection_count);

        CONSOLE_BRIDGE_logDebug("Writing CHUNK_INFO [%llu]: ver=%d pos=%llu start=%d.%d end=%d.%d",
                  (unsigned long long) file_.getOffset(), CHUNK_INFO_VERSION, (unsigned long long) chunk_info.pos,
                  chunk_info.start_time.sec, chunk_info.start_time.nsec,
                  chunk_info.end_time.sec, chunk_info.end_time.nsec);

        writeHeader(header);

        writeDataLength(8 * chunk_connection_count);
        // Write the topic names and counts
        for (map<uint32_t, uint32_t>::const_iterator i = chunk_info.connection_counts.begin(); i != chunk_info.connection_counts.end(); i++) {
            uint32_t connection_id = i->first;
            uint32_t count         = i->second;

            write((char*) &connection_id, 4);
            write((char*) &count, 4);

            CONSOLE_BRIDGE_logDebug("  - %d: %d", connection_id, count);
        }
    }
}

void Bag::readChunkInfoRecord() {
    // Read a CHUNK_INFO header
    rs2rosinternal::Header header;
    uint32_t data_size;
    if (!readHeader(header) || !readDataLength(data_size))
        throw BagFormatException("Error reading CHUNK_INFO record header");
    M_string& fields = *header.getValues();
    if (!isOp(fields, OP_CHUNK_INFO))
        throw BagFormatException("Expected CHUNK_INFO op not found");

    // Check that the chunk info version is current
    uint32_t chunk_info_version;
    readField(fields, VER_FIELD_NAME, true, &chunk_info_version);
    if (chunk_info_version != CHUNK_INFO_VERSION)
        throw BagFormatException( "Expected CHUNK_INFO version " + std::to_string( CHUNK_INFO_VERSION ) + ", read "
                                  + std::to_string( chunk_info_version ) );

    // Read the chunk position, timestamp, and topic count fieldsstd_msgs::Float32
    ChunkInfo chunk_info;
    readField(fields, CHUNK_POS_FIELD_NAME,  true, &chunk_info.pos);
    readField(fields, START_TIME_FIELD_NAME, true,  chunk_info.start_time);
    readField(fields, END_TIME_FIELD_NAME,   true,  chunk_info.end_time);
    uint32_t chunk_connection_count = 0;
    readField(fields, COUNT_FIELD_NAME,      true, &chunk_connection_count);

    CONSOLE_BRIDGE_logDebug("Read CHUNK_INFO: chunk_pos=%llu connection_count=%d start=%d.%d end=%d.%d",
              (unsigned long long) chunk_info.pos, chunk_connection_count,
              chunk_info.start_time.sec, chunk_info.start_time.nsec,
              chunk_info.end_time.sec, chunk_info.end_time.nsec);

    // Read the topic count entries
    for (uint32_t i = 0; i < chunk_connection_count; i ++) {
        uint32_t connection_id, connection_count;
        read((char*) &connection_id,    4);
        read((char*) &connection_count, 4);

        CONSOLE_BRIDGE_logDebug("  %d: %d messages", connection_id, connection_count);

        chunk_info.connection_counts[connection_id] = connection_count;
    }

    chunks_.push_back(chunk_info);
}

// Record I/O

bool Bag::isOp(M_string& fields, uint8_t reqOp) const {
    uint8_t op = 0xFF; // nonexistent op
    readField(fields, OP_FIELD_NAME, true, &op);
    return op == reqOp;
}

void Bag::writeHeader(M_string const& fields) {
	std::vector<uint8_t> header_buffer;
    uint32_t header_len;
    rs2rosinternal::Header::write(fields, header_buffer, header_len);
    write((char*) &header_len, 4);
    write((char*) header_buffer.data(), header_len);
}

void Bag::writeDataLength(uint32_t data_len) {
    write((char*) &data_len, 4);
}

void Bag::appendHeaderToBuffer(Buffer& buf, M_string const& fields) {
	std::vector<uint8_t> header_buffer;
    uint32_t header_len;
    rs2rosinternal::Header::write(fields, header_buffer, header_len);

    uint32_t offset = buf.getSize();

    buf.setSize(buf.getSize() + 4 + header_len);

    memcpy(buf.getData() + offset, &header_len, 4);
    offset += 4;
    memcpy(buf.getData() + offset, header_buffer.data(), header_len);
}

void Bag::appendDataLengthToBuffer(Buffer& buf, uint32_t data_len) {
    uint32_t offset = buf.getSize();

    buf.setSize(buf.getSize() + 4);

    memcpy(buf.getData() + offset, &data_len, 4);
}

//! \todo clean this up
void Bag::readHeaderFromBuffer(Buffer& buffer, uint32_t offset, rs2rosinternal::Header& header, uint32_t& data_size, uint32_t& bytes_read) const {
    assert(buffer.getSize() > 8);

    uint8_t* start = (uint8_t*) buffer.getData() + offset;

    uint8_t* ptr = start;

    // Read the header length
    uint32_t header_len;
    memcpy(&header_len, ptr, 4);
    ptr += 4;

    // Parse the header
    string error_msg;
    bool parsed = header.parse(ptr, header_len, error_msg);
    if (!parsed)
        throw BagFormatException("Error parsing header");
    ptr += header_len;

    // Read the data size
    memcpy(&data_size, ptr, 4);
    ptr += 4;

    bytes_read = static_cast<uint32_t>(ptr - start);
}

void Bag::readMessageDataHeaderFromBuffer(Buffer& buffer, uint32_t offset, rs2rosinternal::Header& header, uint32_t& data_size, uint32_t& total_bytes_read) const {
    (void)buffer;
    total_bytes_read = 0;
    uint8_t op = 0xFF;
    do {
        CONSOLE_BRIDGE_logDebug("reading header from buffer: offset=%d", offset);
        uint32_t bytes_read;
        readHeaderFromBuffer(*current_buffer_, offset, header, data_size, bytes_read);

        offset += bytes_read;
        total_bytes_read += bytes_read;

        readField(*header.getValues(), OP_FIELD_NAME, true, &op);
    }
    while (op == OP_MSG_DEF || op == OP_CONNECTION);

    if (op != OP_MSG_DATA)
        throw BagFormatException("Expected MSG_DATA op not found");
}

bool Bag::readHeader(rs2rosinternal::Header& header) const {
    // Read the header length
    uint32_t header_len;
    read((char*) &header_len, 4);

    // Read the header
    header_buffer_.setSize(header_len);
    read((char*) header_buffer_.getData(), header_len);

    // Parse the header
    string error_msg;
    bool parsed = header.parse(header_buffer_.getData(), header_len, error_msg);
    if (!parsed)
        return false;

    return true;
}

bool Bag::readDataLength(uint32_t& data_size) const {
    read((char*) &data_size, 4);
    return true;
}

M_string::const_iterator Bag::checkField(M_string const& fields, string const& field, unsigned int min_len, unsigned int max_len, bool required) const {
    M_string::const_iterator fitr = fields.find(field);
    if (fitr == fields.end()) {
        if (required)
            throw BagFormatException("Required '" + field + "' field missing");
    }
    else if ((fitr->second.size() < min_len) || (fitr->second.size() > max_len))
        throw BagFormatException( "Field '" + field + "' is wrong size ("
                                  + std::to_string( (uint32_t)fitr->second.size() ) + " bytes)" );

    return fitr;
}

bool Bag::readField(M_string const& fields, string const& field_name, bool required, string& data) const {
    return readField(fields, field_name, 1, UINT_MAX, required, data);
}

bool Bag::readField(M_string const& fields, string const& field_name, unsigned int min_len, unsigned int max_len, bool required, string& data) const {
    M_string::const_iterator fitr = checkField(fields, field_name, min_len, max_len, required);
    if (fitr == fields.end())
        return false;

    data = fitr->second;
    return true;
}

bool Bag::readField(M_string const& fields, string const& field_name, bool required, Time& data) const {
    uint64_t packed_time;
    if (!readField(fields, field_name, required, &packed_time))
        return false;

    uint64_t bitmask = (1LL << 33) - 1;
    data.sec  = (uint32_t) (packed_time & bitmask);
    data.nsec = (uint32_t) (packed_time >> 32);

    return true;
}

std::string Bag::toHeaderString(Time const* field) const {
    uint64_t packed_time = (((uint64_t) field->nsec) << 32) + field->sec;
    return toHeaderString(&packed_time);
}


// Low-level I/O

void Bag::write(string const& s)                  { write(s.c_str(), s.length()); }
void Bag::write(char const* s, std::streamsize n) { file_.write((char*) s, n);    }

void Bag::read(char* b, std::streamsize n) const  { file_.read(b, n);             }
void Bag::seek(uint64_t pos, int origin) const    { file_.seek(pos, origin);      }

} // namespace rosbag
