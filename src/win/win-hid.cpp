// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_WMF_BACKEND

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../types.h"
#include "win-hid.h"
#include "win-helpers.h"

#include <PortableDeviceTypes.h>
//#include <PortableDeviceClassExtension.h>
#include <PortableDevice.h>
#include <Windows.h>
#include <Sensorsapi.h>
#include <sensors.h>
#include <SensAPI.h>
#include <initguid.h>
#include <propkeydef.h>
#include <comutil.h>

#pragma comment(lib, "Sensorsapi.lib")
#pragma comment(lib, "PortableDeviceGuids.lib")

const uint8_t HID_METADATA_SIZE = 8; // bytes

namespace librealsense
{
    namespace platform
    {
        class sensor_events : public ISensorEvents
        {
        public:
            virtual ~sensor_events() = default;

            explicit sensor_events(hid_callback callback) : m_cRef(0), _callback(callback) {}

            STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
            {
                if (ppv == NULL)
                {
                    return E_POINTER;
                }
                if (iid == __uuidof(IUnknown))
                {
                    *ppv = static_cast<IUnknown*>(this);
                }
                else if (iid == __uuidof(ISensorEvents))
                {
                    *ppv = static_cast<ISensorEvents*>(this);
                }
                else
                {
                    *ppv = NULL;
                    return E_NOINTERFACE;
                }
                AddRef();
                return S_OK;
            }

            STDMETHODIMP_(ULONG) AddRef()
            {
                return InterlockedIncrement(&m_cRef);
            }

            STDMETHODIMP_(ULONG) Release()
            {
                ULONG count = InterlockedDecrement(&m_cRef);
                if (count == 0)
                {
                    delete this;
                    return 0;
                }
                return count;
            }

            //
            // ISensorEvents methods.
            //

            STDMETHODIMP OnEvent(
                ISensor *pSensor,
                REFGUID eventID,
                IPortableDeviceValues *pEventData)
            {
                HRESULT hr = S_OK;

                // Handle custom events here.

                return hr;
            }

            STDMETHODIMP OnDataUpdated(ISensor *pSensor, ISensorDataReport *report)
            {
                if (NULL == report ||
                    NULL == pSensor)
                {
                    return E_INVALIDARG;
                }

                BSTR fName{};
                SYSTEMTIME time;
                report->GetTimestamp(&time);

                PROPVARIANT var = {};
                // Custom timestamp low
                auto hr = (report->GetSensorValue(SENSOR_DATA_TYPE_CUSTOM_VALUE1, &var));
                if (FAILED(hr)) return S_OK;
                auto customTimestampLow = var.ulVal;

                // Custom timestamp high
                CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_CUSTOM_VALUE2, &var));
                auto customTimestampHigh = var.ulVal;

                /* Retrieve sensor type - Sensor types are more specific groupings than sensor categories. Sensor type IDs are GUIDs that are defined in Sensors.h */

                SENSOR_TYPE_ID type{};

                CHECK_HR(pSensor->GetType(&type));

                double rawX, rawY, rawZ;


                if (type == SENSOR_TYPE_ACCELEROMETER_3D)
                {
                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_X_G, &var));
                    rawX = var.dblVal;

                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Y_G, &var));
                    rawY = var.dblVal;

                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ACCELERATION_Z_G, &var));
                    rawZ = var.dblVal;

                    static constexpr double accelerator_transform_factor = 1000.0;

                    rawX *= accelerator_transform_factor;
                    rawY *= accelerator_transform_factor;
                    rawZ *= accelerator_transform_factor;
                }
                else if (type == SENSOR_TYPE_GYROMETER_3D)
                {
                    // Raw X
                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_X_DEGREES_PER_SECOND, &var));
                    rawX = var.dblVal;

                    // Raw Y
                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Y_DEGREES_PER_SECOND, &var));
                    rawY = var.dblVal;

                    // Raw Z
                    CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_ANGULAR_VELOCITY_Z_DEGREES_PER_SECOND, &var));
                    rawZ = var.dblVal;

                    static constexpr double gyro_transform_factor = 10.0;

                    rawX *= gyro_transform_factor;
                    rawY *= gyro_transform_factor;
                    rawZ *= gyro_transform_factor;
                }
                else
                {
                    /* Unsupported sensor */
                    return S_FALSE;
                }

                PropVariantClear(&var);

                sensor_data d;
                hid_sensor_data data;

                data.x = rawX;
                data.y = rawY;
                data.z = rawZ;
                data.ts_low = customTimestampLow;
                data.ts_high = customTimestampHigh;

                pSensor->GetFriendlyName(&fName);
                d.sensor.name = CW2A(fName);

                d.fo.pixels = &data;
                d.fo.metadata = &data.ts_low;
                d.fo.metadata_size = HID_METADATA_SIZE;
                d.fo.frame_size = sizeof(data);
                _callback(d);

                return S_OK;
            }

            STDMETHODIMP OnLeave(
                REFSENSOR_ID sensorID)
            {
                HRESULT hr = S_OK;

                // Perform any housekeeping tasks for the sensor that is leaving.
                // For example, if you have maintained a reference to the sensor,
                // release it now and set the pointer to NULL.

                return hr;
            }

            STDMETHODIMP OnStateChanged(
                ISensor* pSensor,
                SensorState state)
            {
                HRESULT hr = S_OK;

                if (NULL == pSensor)
                {
                    return E_INVALIDARG;
                }


                if (state == SENSOR_STATE_READY)
                {
                    wprintf_s(L"\nTime sensor is now ready.");
                }
                else if (state == SENSOR_STATE_ACCESS_DENIED)
                {
                    wprintf_s(L"\nNo permission for the time sensor.\n");
                    wprintf_s(L"Enable the sensor in the control panel.\n");
                }


                return hr;
            }

        private:
            long m_cRef;
            hid_callback _callback;
        };

        void wmf_hid_device::open(const std::vector<hid_profile>&iio_profiles)
        {
            try
            {
                for (auto& profile_to_open : iio_profiles)
                {
                    for (auto& connected_sensor : _connected_sensors)
                    {
                        if (profile_to_open.sensor_name == connected_sensor->get_sensor_name())
                        {
                            /* Set SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL sensor property to profile */
                            HRESULT hr = S_OK;
                            IPortableDeviceValues* pPropsToSet = NULL; // Input
                            IPortableDeviceValues* pPropsReturn = NULL; // Output

                            /* Create the input object */
                            CHECK_HR(CoCreateInstance(__uuidof(PortableDeviceValues), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pPropsToSet)));

                            /* Add the current report interval property */
                            hr = pPropsToSet->SetUnsignedIntegerValue(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, profile_to_open.frequency);
                            if (SUCCEEDED(hr))
                            {
                                // Setting a single property
                                hr = connected_sensor->get_sensor()->SetProperties(pPropsToSet, &pPropsReturn);
                                if (SUCCEEDED(hr))
                                {
                                    _opened_sensors.push_back(connected_sensor);
                                    pPropsReturn->Release();
                                }
                            }

                            pPropsToSet->Release();
                        }
                    }
                }
            }
            catch (...)
            {
                for (auto& connected_sensor : _connected_sensors)
                {
                    connected_sensor.reset();
                }
                _connected_sensors.clear();
                LOG_ERROR("Hid device is busy!");
                throw;
            }
        }

        void wmf_hid_device::close()
        {
            for (auto& open_sensor : _opened_sensors)
            {
                open_sensor.reset();
            }
            _opened_sensors.clear();
        }

        void wmf_hid_device::start_capture(hid_callback callback)
        {
            // Hack, start default profile
            _cb = new sensor_events(callback);
            ISensorEvents* sensorEvents = nullptr;
            CHECK_HR(_cb->QueryInterface(IID_PPV_ARGS(&sensorEvents)));

            for (auto& sensor : _opened_sensors)
            {
                CHECK_HR(sensor->start_capture(sensorEvents));
            }
        }

        void wmf_hid_device::stop_capture()
        {
            for (auto& sensor : _opened_sensors)
            {
                sensor->stop_capture();
            }
            _cb = nullptr;
        }

        std::vector<hid_sensor> wmf_hid_device::get_sensors()
        {
            std::vector<hid_sensor> sensors;

            HRESULT res = S_OK;
            BSTR fName{};

            for (auto& sensor : _opened_sensors)
            {
                sensors.push_back({ sensor->get_sensor_name() });
            }

            SysFreeString(fName);

            return sensors;
        }

        std::vector<uint8_t> wmf_hid_device::get_custom_report_data(const std::string & custom_sensor_name, const std::string & report_name, custom_sensor_report_field report_field)
        {
            return std::vector<uint8_t>();
        }

        void wmf_hid_device::foreach_hid_device(std::function<void(hid_device_info, CComPtr<ISensor>)> action)
        {
            /* Enumerate all HID devices and run action function on each device */
            try
            {
                CComPtr<ISensorManager> pSensorManager = nullptr;
                CComPtr<ISensorCollection> pSensorCollection = nullptr;
                CComPtr<ISensor> pSensor = nullptr;
                ULONG sensorCount = 0;
                HRESULT res{};

                CHECK_HR(CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSensorManager)));

                /* Retrieves a collection containing all sensors associated with category SENSOR_CATEGORY_ALL */
                LOG_HR(res=pSensorManager->GetSensorsByCategory(SENSOR_CATEGORY_ALL, &pSensorCollection));
                if (SUCCEEDED(res))
                {
                    /* Retrieves the count of sensors in the collection */
                    CHECK_HR(pSensorCollection->GetCount(&sensorCount));

                    for (ULONG i = 0; i < sensorCount; i++)
                    {
                        /* Retrieves the sensor at the specified index in the collection */
                        if (SUCCEEDED(pSensorCollection->GetAt(i, &pSensor.p)))
                        {
                            /* Retrieve SENSOR_PROPERTY_FRIENDLY_NAME which is the sensor name that is intended to be seen by the user */
                            BSTR fName{};
                            LOG_HR(res = pSensor->GetFriendlyName(&fName));
                            if (FAILED(res)) fName= L"Unidentified HID sensor";

                            /* Retrieve SENSOR_PROPERTY_PERSISTENT_UNIQUE_ID which is a GUID that uniquely identifies the sensor on the current computer */
                            SENSOR_ID id{};
                            CHECK_HR(pSensor->GetID(&id));

                            /* Retrieve sensor type - Sensor types are more specific groupings than sensor categories. Sensor type IDs are GUIDs that are defined in Sensors.h */
                            SENSOR_TYPE_ID type{};
                            CHECK_HR(pSensor->GetType(&type));

                            CComPtr<IPortableDeviceValues> pValues = nullptr;  // Output
                            hid_device_info info{};

                            /* Retrieves multiple sensor properties */
                            auto hr = pSensor->GetProperties(nullptr, &pValues);
                            if (SUCCEEDED(hr))
                            {
                                /* Get the number of property returned */
                                DWORD propertyCount = 0;
                                hr = pValues->GetCount(&propertyCount);
                                if (SUCCEEDED(hr))
                                {
                                    PROPERTYKEY propertyKey;
                                    PROPVARIANT propertyValue = {};

                                    /* Loop through the properties */
                                    for (DWORD properyIndex = 0; properyIndex < propertyCount; properyIndex++)
                                    {
                                        // Get the value at the current index.
                                        hr = pValues->GetAt(properyIndex, &propertyKey, &propertyValue);
                                        if (SUCCEEDED(hr))
                                        {
                                            if (IsEqualPropertyKey(propertyKey, SENSOR_PROPERTY_DEVICE_PATH))
                                            {
                                                info.device_path = std::string(propertyValue.pwszVal, propertyValue.pwszVal + wcslen(propertyValue.pwszVal));
                                                info.id = std::string(fName, fName + wcslen(fName));

                                                uint16_t vid, pid, mi;
                                                std::string uid;
                                                if (parse_usb_path_multiple_interface(vid, pid, mi, uid, info.device_path))
                                                {
                                                    info.unique_id = "*";
                                                    info.pid = to_string() << std::hex << pid;
                                                    info.vid = to_string() << std::hex << vid;
                                                }
                                            }
                                            if (IsEqualPropertyKey(propertyKey, SENSOR_PROPERTY_SERIAL_NUMBER))
                                            {
                                                info.serial_number = std::string(propertyValue.pwszVal, propertyValue.pwszVal + wcslen(propertyValue.pwszVal));
                                            }
                                        }

                                        PropVariantClear(&propertyValue);
                                    }
                                }
                            }

                            action(info, pSensor);

                            SysFreeString(fName);
                        }
                    }
                }
            }
            catch (...)
            {
                LOG_INFO("Could not enumerate HID devices!");
            }
        }
    }
}

#endif
