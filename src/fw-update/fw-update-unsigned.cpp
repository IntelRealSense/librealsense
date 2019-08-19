// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "fw-update-unsigned.h"
#include <algorithm>

namespace librealsense
{
    std::vector<flash_payload_header> parse_payloads(const std::vector<uint8_t>& flash_buffer, size_t number_of_payloads)
    {
        std::vector<flash_payload_header> rv;
        size_t offset = 0;
        for (int i = 0; i < number_of_payloads; i++)
        {
            flash_payload_header fph = {};
            memcpy(&fph, flash_buffer.data() + i * sizeof(fph), sizeof(fph));
            offset += fph.data_offset + fph.data_size;
            rv.push_back(fph);
        }
        return rv;
    }

    std::vector<flash_table> parse_tables(const std::vector<uint8_t>& flash_buffer, flash_table toc, flash_structure structure)
    {
        std::vector<flash_table> rv;
        auto num_of_tables = toc.header.size / sizeof(uint64_t);

        std::vector<uint32_t> offsets(num_of_tables);
        for (int i = 0; i < num_of_tables; i++)
        {
            memcpy(&offsets[i], toc.data.data() + (i * sizeof(uint64_t)) + 4, sizeof(uint32_t));
        }

        for (auto&& o : offsets)
        {
            flash_table_header h{};
            memcpy(&h, flash_buffer.data() + o, sizeof(h));

            std::vector<uint8_t> d;
            if (h.type == 0 || h.size == 0 || h.size > 0x1000)
                continue;

            d.resize(h.size);
            memcpy(d.data(), flash_buffer.data() + o + sizeof(h), d.size());

            auto types = structure.read_only_sections_types;
            auto it = std::find_if(types.begin(), types.end(), [h](uint16_t t) { return t == h.type; });
            bool read_only = it != types.end();
            flash_table ft = { h, d, o, read_only };

            rv.push_back(ft);
        }
        rv.push_back(toc);
        return rv;
    }

    std::vector<uint8_t> merge_images(flash_info from, flash_info to, const std::vector<uint8_t> image)
    {
        std::vector<uint8_t> rv(image);

        for (auto&& table : from.read_write_section.tables)
        {
            if (!table.read_only)
                continue;
            memcpy(rv.data() + table.offset, &table.header, sizeof(table.header));
            memcpy(rv.data() + table.offset + sizeof(table.header), table.data.data(), table.header.size);
        }

        for (auto&& table : from.read_only_section.tables)
        {
            if (!table.read_only)
                continue;
            memcpy(rv.data() + table.offset, &table.header, sizeof(table.header));
            memcpy(rv.data() + table.offset + sizeof(table.header), table.data.data(), table.header.size);
        }

        return rv;
    }

    flash_table parse_table_of_contents(const std::vector<uint8_t>& flash_buffer, uint32_t toc_offset)
    {
        flash_table rv = {};

        memcpy(&rv.header, flash_buffer.data() + toc_offset, sizeof(rv.header));
        rv.data.resize(rv.header.size);
        memcpy(rv.data.data(), flash_buffer.data() + toc_offset + sizeof(rv.header), rv.data.size());

        rv.read_only = false;
        rv.offset = toc_offset;

        return rv;
    }

    flash_section parse_flash_section(const std::vector<uint8_t>& flash_buffer, flash_table toc, flash_structure s)
    {
        flash_section rv = {};

        rv.table_of_content = toc;
        rv.payloads = parse_payloads(flash_buffer, s.payload_count);
        rv.tables = parse_tables(flash_buffer, toc, s);
        rv.version = toc.header.version;
        rv.app_size = rv.payloads.back().data_offset + rv.payloads.back().data_size;

        return rv;
    }
}
