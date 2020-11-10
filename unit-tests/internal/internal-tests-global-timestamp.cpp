
#include "catch.h"
#include <global_timestamp_reader.h>
#include <device.h>
#include <ds5/ds5-device.h>
#include <ds5/ds5-timestamp.h>

using namespace librealsense;

TEST_CASE("active_object", "[live]")
{
    // log_to_console(RS2_LOG_SEVERITY_DEBUG);
    std::shared_ptr<context> ctx = std::make_shared<context>(backend_type::standard);
    auto&& backend = ctx->get_backend();

    std::vector<std::shared_ptr<device_info>> devs_info = ctx->query_devices(RS2_PRODUCT_LINE_L500 | RS2_PRODUCT_LINE_D400);
    REQUIRE(devs_info.size());
    for (auto dev_info : devs_info)
    {
        std::shared_ptr<device_interface> dev = dev_info->create_device(true);

        if (!Is<global_time_interface>(dev))
        {
            LOG_INFO("!Is<global_time_interface>(dev)");
            continue;
        }

        std::shared_ptr<global_time_interface> gt_dev = As<global_time_interface>(dev);
        std::shared_ptr<time_diff_keeper> tf_keeper = std::make_shared<time_diff_keeper>(&(*gt_dev), 100);
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_backup(new ds5_timestamp_reader(backend.create_time_service()));
        std::unique_ptr<frame_timestamp_reader> ds5_timestamp_reader_metadata(new ds5_timestamp_reader_from_metadata(std::move(ds5_timestamp_reader_backup)));

        // Set 4 global_timestamp_reader objects. The last 2 share the same global_time_option object.
        std::vector< std::shared_ptr<global_time_option> > gtos;
        std::vector< std::shared_ptr<global_timestamp_reader> > gtss;

        for (size_t i; i < 3; i++)
        {
            gtos.push_back(std::shared_ptr<global_time_option>(new global_time_option()));
            gtss.push_back(std::make_shared<global_timestamp_reader>(std::move(ds5_timestamp_reader_metadata), tf_keeper, gtos.back()));
        }
        gtss.push_back(std::make_shared<global_timestamp_reader>(std::move(ds5_timestamp_reader_metadata), tf_keeper, gtos.back()));

        REQUIRE(!tf_keeper->is_running());

        // Test 1: test disabling option: 
        //         start sensor 0, disable option 0, stop sensor 0
        gtos[0]->set(1);
        gtss[0]->sensor_enable_time_diff_keeper(true);
        REQUIRE(tf_keeper->is_running());
        gtos[0]->set(0);
        REQUIRE(!tf_keeper->is_running());
        gtss[0]->sensor_enable_time_diff_keeper(false);
        REQUIRE(!tf_keeper->is_running());

        // Test 2: test disabling option shared by 2 global_timestamp_reader objects:
        //         start sensors 2,3 disable option 0 then option 1 then option 2
        gtos[0]->set(1);
        gtos[1]->set(1);
        gtos[2]->set(1);
        gtss[2]->sensor_enable_time_diff_keeper(true);
        gtss[3]->sensor_enable_time_diff_keeper(true);
        REQUIRE(tf_keeper->is_running());
        gtos[0]->set(0);
        REQUIRE(tf_keeper->is_running());
        gtos[1]->set(0);
        REQUIRE(tf_keeper->is_running());
        gtos[2]->set(0);
        REQUIRE(!tf_keeper->is_running());

        // Test 3: test stopping sensor:
        //         start sensor 1 stop sensor 1
        gtos[1]->set(1);
        gtss[1]->sensor_enable_time_diff_keeper(true);
        REQUIRE(tf_keeper->is_running());
        gtss[1]->sensor_enable_time_diff_keeper(false);
        REQUIRE(!tf_keeper->is_running());

        // Test 4: test stopping sensor 1 does not influence sensor 2:
        //         start sensor 0, start sensor 1, stop sensor 0, stop sensor 1
        gtos[0]->set(1);
        gtos[1]->set(1);
        gtss[0]->sensor_enable_time_diff_keeper(true);
        REQUIRE(tf_keeper->is_running());
        gtss[1]->sensor_enable_time_diff_keeper(true);
        REQUIRE(tf_keeper->is_running());
        gtss[0]->sensor_enable_time_diff_keeper(false);
        REQUIRE(tf_keeper->is_running());
        gtss[1]->sensor_enable_time_diff_keeper(false);
        REQUIRE(!tf_keeper->is_running());
    }

    //std::shared_ptr<time_diff_keeper> tf_keeper(std::make_shared<time_diff_keeper>(this, 100))
}