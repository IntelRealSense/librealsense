// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <stdlib.h>

#include <map>
#include <vector>
#include <iostream>
#include <sstream>      // std::stringstream

#include <jpeg.h>

// #define JPEG_DUMP

#define JPEG_ABBREVIATED
#ifdef JPEG_ABBREVIATED

int jpeg::compress(uint8_t* in, int width, int height, uint8_t* out, uint32_t size)
{
    struct jpeg_compress_struct m_cinfo = {0};
    struct jpeg_error_mgr m_jerr = {0};

    m_cinfo.err = jpeg_std_error(&m_jerr);
    jpeg_create_compress(&m_cinfo);

    m_cinfo.image_width      = width;
    m_cinfo.image_height     = height;
    m_cinfo.input_components = 3;         // YUYV and UYVY are 2 bpp, converted to YUV that is 3 bpp.
    m_cinfo.in_color_space   = JCS_YCbCr; // bypass color conversion

    jpeg_set_defaults(&m_cinfo);          // sets m_cinfo.jpeg_color_space = JCS_YCbCr;

    m_cinfo.dct_method = JDCT_FASTEST;    // we want fastest processing as possible
    m_cinfo.arith_code = FALSE;           // Huffman coding is much better than arithmetic
    m_cinfo.write_JFIF_header = FALSE;    // no header, RTP should create one
    // m_cinfo.restart_in_rows = 1;          // one restart marker per one row of MCUs
    m_cinfo.restart_interval = 1;         // one restart marker per one MCU

    // set to use default quantization tables
    jpeg_set_quality(&m_cinfo, 97, TRUE);

    // // dump the tables into the file
    // FILE* file = fopen("/tmp/tables", "wb");
    // jpeg_stdio_dest(&m_cinfo, file);
    // jpeg_write_tables(&m_cinfo);
    // fclose(file);

#if 0
std::cout << "JPEG will be generated using " << ((m_cinfo.arith_code == true) ? "arithmetic" : "Huffman") << " coding, "
                                             << "4:" << m_cinfo.comp_info[0].v_samp_factor << ":" << m_cinfo.comp_info[0].h_samp_factor << " subsumpling, "
                                             << ((m_cinfo.restart_interval == 0) ? "without" : "with") << " restart MCUs, and "
                                             << ((m_cinfo.restart_in_rows  == 0) ? "without" : "with") << " restart rows\n"; 
#endif

    // put the image into the memory
    uint8_t* out_data = out;
    long unsigned int out_size = size;
    jpeg_mem_dest(&m_cinfo, &out_data, &out_size);

    // do not emit tables
    jpeg_suppress_tables(&m_cinfo, TRUE);

    // create abbreviated image
    jpeg_start_compress(&m_cinfo, FALSE);

    std::vector<uint8_t> row(m_cinfo.image_width * m_cinfo.input_components);
    JSAMPROW row_pointer[1];
    row_pointer[0] = &row[0];
    uint8_t* buffer = in;
    while(m_cinfo.next_scanline < m_cinfo.image_height)
    {
        for(int i = 0; i < m_cinfo.image_width; i += 2)
        {
            row[i * 3 + 0] = buffer[i * 2 + 0]; // Y
            row[i * 3 + 1] = buffer[i * 2 + 1]; // U
            row[i * 3 + 2] = buffer[i * 2 + 3]; // V
            row[i * 3 + 3] = buffer[i * 2 + 2]; // Y
            row[i * 3 + 4] = buffer[i * 2 + 1]; // U
            row[i * 3 + 5] = buffer[i * 2 + 3]; // V
        }
        buffer += m_cinfo.image_width * 2;

        jpeg_write_scanlines(&m_cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&m_cinfo);
    jpeg_destroy_compress(&m_cinfo);

#ifdef JPEG_DUMP
    static int fnum = 0;
    std::stringstream fname;
    fname << "/tmp/cmp" << ++fnum << ".jpeg";

    FILE* dump = fopen(fname.str().c_str(), "wb");
    if (dump) {
        fwrite(out, 1, out_size, dump);
        fclose(dump);
    }
#endif
    return out_size;
}

int jpeg::decompress(unsigned char* in, int in_size, unsigned char* out, uint32_t out_size)
{
    struct jpeg_decompress_struct m_dinfo = {0};
    struct jpeg_error_mgr m_jerr = {0};

#ifdef JPEG_DUMP
    static int fnum = 0;
    std::stringstream fname;
    fname << "/tmp/dec" << ++fnum << ".jpeg";

    FILE* dump = fopen(fname.str().c_str(), "wb");
    if (dump) {
        fwrite(in, 1, in_size, dump);
        fclose(dump);
    }
#endif

    m_dinfo.err = jpeg_std_error(&m_jerr);
    jpeg_create_decompress(&m_dinfo);

    // read the tables from the file
    uint8_t tables[] = {
        0xFF, 0xD8, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x03, 0x02, 0x02, 0x02, 
        0x02, 0x03, 0x03, 0x04, 0x04, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x06, 0x05, 0x04, 0x04, 0x05, 
        0x04, 0x03, 0x03, 0x05, 0x07, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x04, 0x05, 0x07, 0x07, 
        0x07, 0x06, 0x07, 0x06, 0x06, 0x06, 0x06, 0xFF, 0xDB, 0x00, 0x43, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x03, 0x02, 0x02, 0x03, 0x06, 0x04, 0x03, 0x04, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0xFF, 0xC4, 0x00, 0x1F, 
        0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 
        0xB5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 
        0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 
        0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 
        0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 
        0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 
        0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 
        0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 
        0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 
        0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 
        0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 
        0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 
        0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xC4, 0x00, 0x1F, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 
        0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 
        0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 
        0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 
        0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 
        0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 
        0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 
        0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 
        0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 
        0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 
        0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 
        0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 
        0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xD9
    };
    // FILE* file = fopen("/tmp/tables", "rb");
    // fread(tables, 1, sizeof(tables), file);
    // fclose(file);

    // read the header from the memory
    jpeg_mem_src(&m_dinfo, tables, sizeof(tables));
    jpeg_read_header(&m_dinfo, FALSE);

    // read the image from the memory
    jpeg_mem_src(&m_dinfo, in, in_size);
    jpeg_read_header(&m_dinfo, TRUE);

    // bypass the color conversion
    m_dinfo.out_color_space  = JCS_YCbCr;  
    m_dinfo.jpeg_color_space = JCS_YCbCr;

    std::vector<uint8_t> row(out_size);
    JSAMPROW row_pointer[1];
    row_pointer[0] = &row[0];

    m_dinfo.dct_method = JDCT_FASTEST;
    
    jpeg_start_decompress(&m_dinfo);
    while(m_dinfo.output_scanline < m_dinfo.output_height)
    {
        int numLines = jpeg_read_scanlines(&m_dinfo, row_pointer, 1);
        for(int i = 0; i < m_dinfo.output_width; i += 2)
        {
            out[i * 2 + 0] = row[i * 3 + 0]; // Y
            out[i * 2 + 1] = row[i * 3 + 1]; // U
            out[i * 2 + 2] = row[i * 3 + 3]; // Y
            out[i * 2 + 3] = row[i * 3 + 2]; // V
        }
        out += m_dinfo.output_width * 2;
    }
    jpeg_finish_decompress(&m_dinfo);
    jpeg_destroy_decompress(&m_dinfo);

    return 0;
}

#else

int jpeg::compress(uint8_t* in, int width, int height, uint8_t* out, uint32_t size)
{
    struct jpeg_compress_struct m_cinfo = {0};
    struct jpeg_error_mgr m_jerr = {0};

    uint8_t* out_data = out;
    long unsigned int out_size = size;

    m_cinfo.err = jpeg_std_error(&m_jerr);
    jpeg_create_compress(&m_cinfo);
    jpeg_mem_dest(&m_cinfo, &out_data, &out_size);

    m_cinfo.image_width = width;
    m_cinfo.image_height = height;
    m_cinfo.input_components = 3; //yuyv and uyvy is 2 bpp, converted to yuv that is 3 bpp.
    m_cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&m_cinfo);
    jpeg_set_quality(&m_cinfo, 97, true);
    m_cinfo.dct_method = JDCT_FASTEST;
    // m_cinfo.arith_code == true;        // Huffman coding is much better than arithmetic
    // m_cinfo.write_JFIF_header = false; // no header, RTP should create one

#if 0
std::cout << "JPEG will be generated using " << ((m_cinfo.arith_code == true) ? "arithmetic" : "Huffman") << " coding, "
                                             << "4:" << m_cinfo.comp_info[0].v_samp_factor << ":" << m_cinfo.comp_info[0].h_samp_factor << " subsumpling, "
                                             << ((m_cinfo.restart_interval == 0) ? "without" : "with") << " restart MCUs, and "
                                             << ((m_cinfo.restart_in_rows  == 0) ? "without" : "with") << " restart rows\n"; 
#endif

    jpeg_start_compress(&m_cinfo, TRUE);

    std::vector<uint8_t> row(m_cinfo.image_width * m_cinfo.input_components);
    JSAMPROW row_pointer[1];
    row_pointer[0] = &row[0];
    uint8_t* buffer = in;
    while(m_cinfo.next_scanline < m_cinfo.image_height)
    {
        for(int i = 0; i < m_cinfo.image_width; i += 2)
        {
            row[i * 3 + 0] = buffer[i * 2 + 0]; // Y
            row[i * 3 + 1] = buffer[i * 2 + 1]; // U
            row[i * 3 + 2] = buffer[i * 2 + 3]; // V
            row[i * 3 + 3] = buffer[i * 2 + 2]; // Y
            row[i * 3 + 4] = buffer[i * 2 + 1]; // U
            row[i * 3 + 5] = buffer[i * 2 + 3]; // V
        }
        buffer += m_cinfo.image_width * 2;

        jpeg_write_scanlines(&m_cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&m_cinfo);
    jpeg_destroy_compress(&m_cinfo);

    return out_size;
}

int jpeg::decompress(unsigned char* in, int in_size, unsigned char* out, uint32_t out_size)
{
    struct jpeg_decompress_struct m_dinfo = {0};
    struct jpeg_error_mgr m_jerr = {0};

    m_dinfo.err = jpeg_std_error(&m_jerr);
    jpeg_create_decompress(&m_dinfo);
    // m_dinfo.out_color_space = JCS_UNKNOWN;

    jpeg_mem_src(&m_dinfo, in, in_size);
    jpeg_read_header(&m_dinfo, TRUE);

    std::vector<uint8_t> row(out_size);
    JSAMPROW row_pointer[1];
    row_pointer[0] = &row[0];

    m_dinfo.dct_method = JDCT_FASTEST;
    m_dinfo.out_color_space = JCS_YCbCr;

    jpeg_start_decompress(&m_dinfo);
    while(m_dinfo.output_scanline < m_dinfo.output_height)
    {
        int numLines = jpeg_read_scanlines(&m_dinfo, row_pointer, 1);
        for(int i = 0; i < m_dinfo.output_width; i += 2)
        {
            out[i * 2 + 0] = row[i * 3 + 0]; // Y
            out[i * 2 + 1] = row[i * 3 + 1]; // U
            out[i * 2 + 2] = row[i * 3 + 3]; // Y
            out[i * 2 + 3] = row[i * 3 + 2]; // V
        }
        out += m_dinfo.output_width * 2;
    }
    jpeg_finish_decompress(&m_dinfo);
    jpeg_destroy_decompress(&m_dinfo);

    return 0;
}
#endif