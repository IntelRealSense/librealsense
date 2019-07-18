// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"

#include <stdint.h>
#include <vector>
#include <memory>

namespace librealsense
{
    struct flash_structure
    {
        uint16_t payload_count;
        std::vector<uint16_t> read_only_sections_types;
    };

    struct flash_payload_header
    {
        uint16_t spi_clk_divider;
        uint16_t type;
        uint32_t code_offset;
        uint32_t code_size;
        uint32_t code_address;
        uint32_t code_crc;
        uint32_t data_offset;
        uint32_t data_size;
        uint32_t data_address;
        uint32_t data_crc;
        uint32_t read_16_bit_enabled;
        uint8_t  fw_header[16];
        uint32_t crc_disable;
        uint32_t header_crc;
    };

    struct flash_table_header
    {
        uint16_t version;
        uint16_t type;
        uint32_t size;
        uint32_t reserved;
        uint32_t table_crc;
    };

    struct flash_payload
    {
        flash_payload_header header;
        std::vector<uint8_t> data;
    };

    struct flash_table
    {
        flash_table_header header;
        std::vector<uint8_t> data;
        uint32_t offset;
        bool read_only;

    };

    struct flash_info_header
    {
        uint32_t read_only_start_address;
        uint32_t read_only_size;
        uint32_t read_write_start_address;
        uint32_t read_write_size;
    };

    struct flash_section
    {
        uint16_t version;
        uint32_t offset;
        uint32_t app_size;
        flash_table table_of_content;
        std::vector<flash_payload_header> payloads;
        std::vector<flash_table> tables;
    };

    struct flash_info
    {
        flash_info_header header;
        flash_section read_write_section;
        flash_section read_only_section;
    };

    std::vector<flash_payload_header> parse_payloads(const std::vector<uint8_t>& flash_buffer, size_t number_of_payloads);
    std::vector<flash_table> parse_tables(const std::vector<uint8_t>& flash_buffer, flash_table toc, flash_structure structure);
    flash_table parse_table_of_contents(const std::vector<uint8_t>& flash_buffer, uint32_t toc_offset);
    std::vector<uint8_t> merge_images(flash_info from, flash_info to, const std::vector<uint8_t> image);
    flash_section parse_flash_section(const std::vector<uint8_t>& flash_buffer, flash_table toc, flash_structure s);
}
