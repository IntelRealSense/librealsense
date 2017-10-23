// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "tm-factory.h"
#include "tm-device.h"

using namespace perc;

#define TM2_PID 0xabcd 
// TODO: Sergey

namespace librealsense
{
	class tm2_context
	{
	public:
		tm2_context();

		std::shared_ptr<perc::TrackingManager> get_manager() const
		{
			return _manager;
		}

		std::vector<perc::TrackingDevice*> query_devices() const
		{
			return _devices;
		}
	private:
		friend class connect_disconnect_listener;
		std::shared_ptr<perc::TrackingManager::Listener> _listener;
		std::shared_ptr<perc::TrackingManager> _manager;
		std::vector<perc::TrackingDevice*> _devices;
	};

	class connect_disconnect_listener : public TrackingManager::Listener
	{
	public:
		connect_disconnect_listener(tm2_context* owner) : _owner(owner) {}

		virtual void onStateChanged(TrackingManager::EventType state, TrackingDevice* dev)
		{
			switch (state)
			{
			case TrackingManager::ATTACH:
				_owner->_devices.push_back(dev);
				LOG_INFO("TM2 Device Attached - " << dev);
				break;

			case TrackingManager::DETACH:
				LOG_INFO("TM2 Device Detached");
				// TODO: Sergey
				// Need to clarify if the device pointer has value
				break;
			}
		};

		virtual void onError(TrackingManager::Error error, TrackingDevice* dev) override
		{
			LOG_ERROR("Error occured while connecting device:" << dev << " Error: 0x" << std::hex << error);
		}

	private:
		tm2_context* _owner;
	};

	tm2_context::tm2_context()
	{
		_listener = std::make_shared<connect_disconnect_listener>(this);
		_manager = std::shared_ptr<TrackingManager>(
			perc::TrackingManager::CreateInstance(_listener.get()));
		if (_manager == nullptr)
		{
			LOG_ERROR("Failed to create TrackingManager");
		}
		auto version = _manager->version();
		LOG_INFO("LibTm version 0x" << std::hex << version);
	}

	std::shared_ptr<device_interface> tm2_info::create(std::shared_ptr<context> ctx,
		bool register_device_notifications) const
	{
		platform::backend_device_group g{ { _usb } };
		return std::make_shared<tm2_device>(_manager, _dev, _ctx, g);
	}

	std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
		std::shared_ptr<context> ctx,
		platform::backend_device_group& group)
	{
		static tm2_context context;
		
		std::vector<platform::usb_device_info> chosen;
		std::vector<std::shared_ptr<device_info>> results;

		auto correct_pid = filter_by_product(group.usb_devices, { TM2_PID });

		auto tm_devices = context.query_devices();

		if (correct_pid.size() == tm_devices.size() && correct_pid.size() == 1)
		{
			auto usb_dev = correct_pid.front();
			chosen.push_back(usb_dev);
			auto result = std::make_shared<tm2_info>(context.get_manager(), tm_devices.front(), ctx, usb_dev);
		}
		else
		{
			LOG_WARNING("At the moment only single TM2 device is supported");
		}
		
		trim_device_list(group.usb_devices, chosen);

		return results;
	}
}
