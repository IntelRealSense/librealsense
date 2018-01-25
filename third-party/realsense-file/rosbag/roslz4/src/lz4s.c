/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, Ben Charrow
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#include "roslz4/lz4s.h"

#include "xxhash.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if 0
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

// magic numbers
const uint32_t kMagicNumber = 0x184D2204;
const uint32_t kEndOfStream = 0x00000000;

// Bitmasks
const uint8_t k1Bits = 0x01;
const uint8_t k2Bits = 0x03;
const uint8_t k3Bits = 0x07;
const uint8_t k4Bits = 0x0F;
const uint8_t k8Bits = 0xFF;

uint32_t readUInt32(unsigned char *buffer) {
  return ((buffer[0] <<  0) | (buffer[1] <<  8) |
          (buffer[2] << 16) | (buffer[3] << 24));
}

void writeUInt32(unsigned char *buffer, uint32_t val) {
  buffer[0] = val & 0xFF;
  buffer[1] = (val >> 8) & 0xFF;
  buffer[2] = (val >> 16) & 0xFF;
  buffer[3] = (val >> 24) & 0xFF;
}
#undef min
int min(int a, int b) {
  return a < b ? a : b;
}

/*========================== Low level compression ==========================*/

typedef struct {
  int block_independence_flag;
  int block_checksum_flag;
  int stream_checksum_flag;

  char *buffer;
  int buffer_size;
  int buffer_offset;

  int finished; // 1 if done compressing/decompressing; 0 otherwise

  void* xxh32_state;

  // Compression state
  int wrote_header;

  // Decompression state
  char header[10];
  uint32_t block_size; // Size of current block
  int block_size_read; // # of bytes read for current block_size
  int block_uncompressed; // 1 if block is uncompressed, 0 otherwise
  uint32_t stream_checksum; // Storage for checksum
  int stream_checksum_read; // # of bytes read for stream_checksum
} stream_state;

void advanceInput(roslz4_stream *str, int nbytes) {
  str->input_next += nbytes;
  str->input_left -= nbytes;
  str->total_in += nbytes;
}

void advanceOutput(roslz4_stream *str, int nbytes) {
  str->output_next += nbytes;
  str->output_left -= nbytes;
  str->total_out += nbytes;
}

void fillUInt32(roslz4_stream *str, uint32_t *dest_val, int *offset) {
  char *dest = (char*) dest_val;
  int to_copy = min(4 - *offset, str->input_left);
  memcpy(dest + *offset, str->input_next, to_copy);
  advanceInput(str, to_copy);
  *offset += to_copy;
}

int writeHeader(roslz4_stream *str) {
  if (str->output_left < 7) {
    return ROSLZ4_OUTPUT_SMALL; // Output must have 7 bytes
  }

  stream_state *state = str->state;
  writeUInt32((unsigned char*) str->output_next, kMagicNumber);
  int version = 1;
  char *out = str->output_next;
  *(out+4)  = ((unsigned)version                        & k2Bits) << 6;
  *(out+4) |= ((unsigned)state->block_independence_flag & k1Bits) << 5;
  *(out+4) |= ((unsigned)state->block_checksum_flag     & k1Bits) << 4;
  *(out+4) |= ((unsigned)state->stream_checksum_flag    & k1Bits) << 2;
  *(out+5)  = ((unsigned)str->block_size_id             & k3Bits) << 4;

  // Checksum: 2nd byte of hash of header flags
  unsigned char checksum = (XXH32(str->output_next + 4, 2, 0) >> 8) & k8Bits;
  *(str->output_next+6) = checksum;

  advanceOutput(str, 7);
  DEBUG("writeHeader() Put 7 bytes in output\n");

  return ROSLZ4_OK;
}

int writeEOS(roslz4_stream *str) {
  if (str->output_left < 8) {
    return ROSLZ4_OUTPUT_SMALL;
  }

  stream_state *state = str->state;
  state->finished = 1;
  writeUInt32((unsigned char*) str->output_next, kEndOfStream);
  advanceOutput(str, 4);

  uint32_t stream_checksum = XXH32_digest(state->xxh32_state);
  writeUInt32((unsigned char*) str->output_next, stream_checksum);
  advanceOutput(str, 4);
  state->xxh32_state = NULL;

  DEBUG("writeEOS() Wrote 8 bytes to output %i\n", str->output_left);
  return ROSLZ4_STREAM_END;
}

// If successfull, number of bytes written to output
// If error, LZ4 return code
int bufferToOutput(roslz4_stream *str) {
  stream_state *state = str->state;
  int uncomp_size = state->buffer_offset;
  if (state->buffer_offset == 0) {
    return 0; // No data to flush
  } else if (str->output_left - 4 < uncomp_size) {
    DEBUG("bufferToOutput() Not enough space left in output\n");
    return ROSLZ4_OUTPUT_SMALL;
  }

  DEBUG("bufferToOutput() Flushing %i bytes, %i left in output\n",
        state->buffer_offset, str->output_left);

  // Shrink output by 1 to detect if data is not compressible
  uint32_t comp_size = LZ4_compress_default(state->buffer,
                                            str->output_next + 4,
                                            state->buffer_offset,
                                            uncomp_size - 1);
  uint32_t wrote;
  if (comp_size > 0) {
    DEBUG("bufferToOutput() Compressed to %i bytes\n", comp_size);
    // Write compressed data size
    wrote = 4 + comp_size;
    writeUInt32((unsigned char*)str->output_next, comp_size);
  } else {
    // Write uncompressed data
    DEBUG("bufferToOutput() Can't compress, copying input\n");
    memcpy(str->output_next + 4, state->buffer, uncomp_size);
    // Write uncompressed data size.  Signal data is uncompressed with high
    // order bit; won't confuse decompression because max block size is < 2GB
    wrote = 4 + uncomp_size;
    writeUInt32((unsigned char*) str->output_next, uncomp_size | 0x80000000);
  }

  advanceOutput(str, wrote);
  state->buffer_offset -= uncomp_size;

  DEBUG("bufferToOutput() Ate %i from buffer, wrote %i to output (%i)\n",
        uncomp_size, wrote, str->output_left);
  return wrote;
}

// Copy as much data as possible from input to internal buffer
// Return number of bytes written if successful, LZ4 error code on error
int inputToBuffer(roslz4_stream *str) {
  stream_state *state = str->state;
  if (str->input_left == 0 ||
      state->buffer_size == state->buffer_offset) {
    return 0;
  }
  int buffer_left = state->buffer_size - state->buffer_offset;
  int to_copy = min(str->input_left, buffer_left);

  int ret = XXH32_update(state->xxh32_state, str->input_next, to_copy);
  if (ret == XXH_ERROR) { return ROSLZ4_ERROR; }

  memcpy(state->buffer + state->buffer_offset, str->input_next, to_copy);
  advanceInput(str, to_copy);
  state->buffer_offset += to_copy;

  DEBUG("inputToBuffer() Wrote % 5i bytes to buffer (size=% 5i)\n",
        to_copy, state->buffer_offset);
  return to_copy;
}

int streamStateAlloc(roslz4_stream *str) {
  stream_state *state = (stream_state*) malloc(sizeof(stream_state));
  if (state == NULL) {
    return ROSLZ4_MEMORY_ERROR; // Allocation of state failed
  }
  str->state = state;

  str->block_size_id = -1;
  state->block_independence_flag = 1;
  state->block_checksum_flag = 0;
  state->stream_checksum_flag = 1;

  state->finished = 0;

  state->xxh32_state = XXH32_init(0);
  state->stream_checksum = 0;
  state->stream_checksum_read = 0;

  state->wrote_header = 0;

  state->buffer_offset = 0;
  state->buffer_size = 0;
  state->buffer = NULL;

  state->block_size = 0;
  state->block_size_read = 0;
  state->block_uncompressed = 0;

  str->total_in = 0;
  str->total_out = 0;

  return ROSLZ4_OK;
}

int streamResizeBuffer(roslz4_stream *str, int block_size_id) {
  stream_state *state = str->state;
  if (!(4 <= block_size_id && block_size_id <= 7)) {
    return ROSLZ4_PARAM_ERROR; // Invalid block size
  }

  str->block_size_id = block_size_id;
  state->buffer_offset = 0;
  state->buffer_size = roslz4_blockSizeFromIndex(str->block_size_id);
  state->buffer = (char*) malloc(sizeof(char) * state->buffer_size);
  if (state->buffer == NULL) {
    return ROSLZ4_MEMORY_ERROR; // Allocation of buffer failed
  }
  return ROSLZ4_OK;
}

void streamStateFree(roslz4_stream *str) {
  stream_state *state = str->state;
  if (state != NULL) {
    if (state->buffer != NULL) {
      free(state->buffer);
    }
    if (state->xxh32_state != NULL) {
      XXH32_digest(state->xxh32_state);
    }
    free(state);
    str->state = NULL;
  }
}

int roslz4_blockSizeFromIndex(int block_id) {
  return (1 << (8 + (2 * block_id)));
}

int roslz4_compressStart(roslz4_stream *str, int block_size_id) {
  int ret = streamStateAlloc(str);
  if (ret < 0) { return ret; }
  return streamResizeBuffer(str, block_size_id);
}

int roslz4_compress(roslz4_stream *str, int action) {
  int ret;
  stream_state *state = str->state;
  if (action != ROSLZ4_RUN && action != ROSLZ4_FINISH) {
    return ROSLZ4_PARAM_ERROR; // Unrecognized compression action
  } else if (state->finished) {
    return ROSLZ4_ERROR; // Cannot call action on finished stream
  }

  if (!state->wrote_header) {
    ret = writeHeader(str);
    if (ret < 0) { return ret; }
    state->wrote_header = 1;
  }

  // Copy input to internal buffer, compressing when full or finishing stream
  int read = 0, wrote = 0;
  do {
    read = inputToBuffer(str);
    if (read < 0) { return read; }

    wrote = 0;
    if (action == ROSLZ4_FINISH || state->buffer_offset == state->buffer_size) {
      wrote = bufferToOutput(str);
      if (wrote < 0) { return wrote; }
    }
  } while (read > 0 || wrote > 0);

  // Signal end of stream if finishing up, otherwise done
  if (action == ROSLZ4_FINISH) {
    return writeEOS(str);
  } else {
    return ROSLZ4_OK;
  }
}

void roslz4_compressEnd(roslz4_stream *str) {
  streamStateFree(str);
}

/*========================= Low level decompression =========================*/

int roslz4_decompressStart(roslz4_stream *str) {
  return streamStateAlloc(str);
  // Can't allocate internal buffer, block size is unknown until header is read
}

// Return 1 if header is present, 0 if more data is needed,
// LZ4 error code (< 0) if error
int processHeader(roslz4_stream *str) {
  stream_state *state = str->state;
  if (str->total_in >= 7) {
    return 1;
  }
  // Populate header buffer
  int to_copy = min(7 - str->total_in, str->input_left);
  memcpy(state->header + str->total_in, str->input_next, to_copy);
  advanceInput(str, to_copy);
  if (str->total_in < 7) {
    return 0;
  }

  // Parse header buffer
  unsigned char *header = (unsigned char*) state->header;
  uint32_t magic_number = readUInt32(header);
  if (magic_number != kMagicNumber) {
    return ROSLZ4_DATA_ERROR; // Stream does not start with magic number
  }
  // Check descriptor flags
  int version                 = (header[4] >> 6) & k2Bits;
  int block_independence_flag = (header[4] >> 5) & k1Bits;
  int block_checksum_flag     = (header[4] >> 4) & k1Bits;
  int stream_size_flag        = (header[4] >> 3) & k1Bits;
  int stream_checksum_flag    = (header[4] >> 2) & k1Bits;
  int reserved1               = (header[4] >> 1) & k1Bits;
  int preset_dictionary_flag  = (header[4] >> 0) & k1Bits;

  int reserved2               = (header[5] >> 7) & k1Bits;
  int block_max_id            = (header[5] >> 4) & k3Bits;
  int reserved3               = (header[5] >> 0) & k4Bits;

  // LZ4 standard requirements
  if (version != 1) {
    return ROSLZ4_DATA_ERROR; // Wrong version number
  }
  if (reserved1 != 0 || reserved2 != 0 || reserved3 != 0) {
    return ROSLZ4_DATA_ERROR; // Reserved bits must be 0
  }
  if (!(4 <= block_max_id && block_max_id <= 7)) {
    return ROSLZ4_DATA_ERROR; // Invalid block size
  }

  // Implementation requirements
  if (stream_size_flag != 0) {
    return ROSLZ4_DATA_ERROR; // Stream size not supported
  }
  if (preset_dictionary_flag != 0) {
    return ROSLZ4_DATA_ERROR; // Dictionary not supported
  }
  if (block_independence_flag != 1) {
    return ROSLZ4_DATA_ERROR; // Block dependence not supported
  }
  if (block_checksum_flag != 0) {
    return ROSLZ4_DATA_ERROR; // Block checksums not supported
  }
  if (stream_checksum_flag != 1) {
    return ROSLZ4_DATA_ERROR; // Must have stream checksum
  }

  int header_checksum = (XXH32(header + 4, 2, 0) >> 8) & k8Bits;
  int stored_header_checksum = (header[6] >> 0) & k8Bits;
  if (header_checksum != stored_header_checksum) {
    return ROSLZ4_DATA_ERROR; // Header checksum doesn't match
  }

  int ret = streamResizeBuffer(str, block_max_id);
  if (ret == ROSLZ4_OK) {
    return 1;
  } else {
    return ret;
  }
}

// Read block size, return 1 if value is stored in state->block_size 0 otherwise
int readBlockSize(roslz4_stream *str) {
  stream_state *state = str->state;
  if (state->block_size_read < 4) {
    fillUInt32(str, &state->block_size, &state->block_size_read);
    if (state->block_size_read == 4) {
      state->block_size = readUInt32((unsigned char*)&state->block_size);
      state->block_uncompressed = ((unsigned)state->block_size >> 31) & k1Bits;
      state->block_size &= 0x7FFFFFFF;
      DEBUG("readBlockSize() Block size = %i uncompressed = %i\n",
            state->block_size, state->block_uncompressed);
      return 1;
    } else {
      return 0;
    }
  }
  return 1;
}

// Copy at most one blocks worth of data from input to internal buffer.
// Return 1 if whole block has been read, 0 if not, LZ4 error otherwise
int readBlock(roslz4_stream *str) {
  stream_state *state = str->state;
  if (state->block_size_read != 4 || state->block_size == kEndOfStream) {
    return ROSLZ4_ERROR;
  }

  int block_left = state->block_size - state->buffer_offset;
  int to_copy = min(str->input_left, block_left);
  memcpy(state->buffer + state->buffer_offset, str->input_next, to_copy);
  advanceInput(str, to_copy);
  state->buffer_offset += to_copy;
  DEBUG("readBlock() Read %i bytes from input (block = %i/%i)\n",
        to_copy, state->buffer_offset, state->block_size);
  return state->buffer_offset == state->block_size;
}

int decompressBlock(roslz4_stream *str) {
  stream_state *state = str->state;
  if (state->block_size_read != 4 || state->block_size != state->buffer_offset) {
    // Internal error: Can't decompress block, it's not in buffer
    return ROSLZ4_ERROR;
  }

  if (state->block_uncompressed) {
    if (str->output_left > 0 && (unsigned int)str->output_left >= state->block_size) {
      memcpy(str->output_next, state->buffer, state->block_size);
      int ret = XXH32_update(state->xxh32_state, str->output_next,
                             state->block_size);
      if (ret == XXH_ERROR) { return ROSLZ4_ERROR; }
      advanceOutput(str, state->block_size);
      state->block_size_read = 0;
      state->buffer_offset = 0;
      return ROSLZ4_OK;
    } else {
      return ROSLZ4_OUTPUT_SMALL;
    }
  } else {
    int decomp_size;
    decomp_size = LZ4_decompress_safe(state->buffer, str->output_next,
                                      state->block_size, str->output_left);
    if (decomp_size < 0) {
      if (str->output_left >= state->buffer_size) {
        return ROSLZ4_DATA_ERROR; // Must be a problem with the data stream
      } else {
        // Data error or output is small; increase output to disambiguate
        return ROSLZ4_OUTPUT_SMALL;
      }
    } else {
      int ret = XXH32_update(state->xxh32_state, str->output_next, decomp_size);
      if (ret == XXH_ERROR) { return ROSLZ4_ERROR; }
      advanceOutput(str, decomp_size);
      state->block_size_read = 0;
      state->buffer_offset = 0;
      return ROSLZ4_OK;
    }
  }
}

int readChecksum(roslz4_stream *str) {
  stream_state *state = str->state;
  fillUInt32(str, &state->stream_checksum, &state->stream_checksum_read);
  if (state->stream_checksum_read == 4) {
    state->finished = 1;
    state->stream_checksum = readUInt32((unsigned char*)&state->stream_checksum);
    uint32_t checksum = XXH32_digest(state->xxh32_state);
    state->xxh32_state = NULL;
    if (checksum == state->stream_checksum) {
      return ROSLZ4_STREAM_END;
    } else {
      return ROSLZ4_DATA_ERROR;
    }
  }
  return ROSLZ4_OK;
}

int roslz4_decompress(roslz4_stream *str) {
  stream_state *state = str->state;
  if (state->finished) {
    return ROSLZ4_ERROR; // Already reached end of stream
  }

  // Return if header isn't present or error was encountered
  int ret = processHeader(str);
  if (ret <= 0) {
    return ret;
  }

  // Read in blocks and decompress them as long as there's data to be processed
  while (str->input_left > 0) {
    ret = readBlockSize(str);
    if (ret == 0) { return ROSLZ4_OK; }

    if (state->block_size == kEndOfStream) {
      return readChecksum(str);
    }

    ret = readBlock(str);
    if (ret == 0) { return ROSLZ4_OK; }
    else if (ret < 0) { return ret; }

    ret = decompressBlock(str);
    if (ret < 0) { return ret; }
  }
  return ROSLZ4_OK;
}

void roslz4_decompressEnd(roslz4_stream *str) {
  streamStateFree(str);
}

/*=================== Oneshot compression / decompression ===================*/

int roslz4_buffToBuffCompress(char *input, unsigned int input_size,
                              char *output, unsigned int *output_size,
                              int block_size_id) {
  roslz4_stream stream;
  stream.input_next = input;
  stream.input_left = input_size;
  stream.output_next = output;
  stream.output_left = *output_size;

  int ret;
  ret = roslz4_compressStart(&stream, block_size_id);
  if (ret != ROSLZ4_OK) { return ret; }

  while (stream.input_left > 0 && ret != ROSLZ4_STREAM_END) {
    ret = roslz4_compress(&stream, ROSLZ4_FINISH);
    if (ret == ROSLZ4_ERROR || ret == ROSLZ4_OUTPUT_SMALL) {
      roslz4_compressEnd(&stream);
      return ret;
    }
  }

  *output_size = *output_size - stream.output_left;
  roslz4_compressEnd(&stream);

  if (stream.input_left == 0 && ret == ROSLZ4_STREAM_END) {
    return ROSLZ4_OK; // Success
  } else {
    return ROSLZ4_ERROR; // User did not provide exact buffer
  }
}

int roslz4_buffToBuffDecompress(char *input, unsigned int input_size,
                                char *output, unsigned int *output_size) {
  roslz4_stream stream;
  stream.input_next = input;
  stream.input_left = input_size;
  stream.output_next = output;
  stream.output_left = *output_size;

  int ret;
  ret = roslz4_decompressStart(&stream);
  if (ret != ROSLZ4_OK) { return ret; }

  while (stream.input_left > 0 && ret != ROSLZ4_STREAM_END) {
    ret = roslz4_decompress(&stream);
    if (ret < 0) {
      roslz4_decompressEnd(&stream);
      return ret;
    }
  }

  *output_size = *output_size - stream.output_left;
  roslz4_decompressEnd(&stream);

  if (stream.input_left == 0 && ret == ROSLZ4_STREAM_END) {
    return ROSLZ4_OK; // Success
  } else {
    return ROSLZ4_ERROR; // User did not provide exact buffer
  }
}
