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

#ifndef ROSLZ4_LZ4S_H
#define ROSLZ4_LZ4S_H

#include <lz4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return codes
const int ROSLZ4_MEMORY_ERROR = -5;
const int ROSLZ4_PARAM_ERROR = -4;
const int ROSLZ4_DATA_ERROR = -3;
const int ROSLZ4_OUTPUT_SMALL = -2;
const int ROSLZ4_ERROR = -1;
const int ROSLZ4_OK = 0;
const int ROSLZ4_STREAM_END = 2;

// Actions
const int ROSLZ4_RUN = 0;
const int ROSLZ4_FINISH = 1;

typedef struct {
  char *input_next;
  int input_left;

  char *output_next;
  int output_left;

  int total_in;
  int total_out;

  int block_size_id;

  // Internal use
  void *state;
} roslz4_stream;

// Low level functions
int roslz4_blockSizeFromIndex(int block_id);

int roslz4_compressStart(roslz4_stream *stream, int block_size_id);
int roslz4_compress(roslz4_stream *stream, int action);
void roslz4_compressEnd(roslz4_stream *stream);

int roslz4_decompressStart(roslz4_stream *stream);
int roslz4_decompress(roslz4_stream *stream);
void roslz4_decompressEnd(roslz4_stream *str);

// Oneshot compression / decompression
int roslz4_buffToBuffCompress(char *input, unsigned int input_size,
                              char *output, unsigned int *output_size,
                              int block_size_id);
int roslz4_buffToBuffDecompress(char *input, unsigned int input_size,
                                char *output, unsigned int *output_size);

#ifdef __cplusplus
}
#endif

#endif // ROSLZ4_LZ4S_H
