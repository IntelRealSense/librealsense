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

                // Raw X
                CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_CUSTOM_VALUE3, &var));
                auto rawX = var.iVal;

                // Raw Y
                CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_CUSTOM_VALUE4, &var));
                auto rawY = var.iVal;

                // Raw Z
                CHECK_HR(report->GetSensorValue(SENSOR_DATA_TYPE_CUSTOM_VALUE5, &var));
                auto rawZ = var.iVal;

                PropVariantClear(&var);

                sensor_data d;
                hid_sensor_data data;

                data.x = rawX;
                data.y = rawY;
                data.z = rawZ;
                data.ts_low = customTimestampLow;
                data.ts_high = customTimestampHigh;
                d.sensor.name = "";

                d.fo.pixels = &data;
                d.fo.metadata = NULL;
                d.fo.frame_size = sizeof(data);
                _callback(d);

                return S_OK;
            }

            STDMETHODIMP OnLeave(
                REFSENSOR_ID sensorID)
            {
                HRESULT hr = S_OK;

                // Peform any housekeeping tasks for the sensor that is leaving.
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
        }

        void wmf_hid_device::close()
        {
        }

        void wmf_hid_device::stop_capture()
        {
            _sensor->SetEventSink(NULL);
            _cb = nullptr;

        }

        void wmf_hid_device::start_capture(hid_callback callback)
        {
            // Hack, start default profile
            _cb = new sensor_events(callback);
            ISensorEvents* sensorEvents = nullptr;
            CHECK_HR(_cb->QueryInterface(IID_PPV_ARGS(&sensorEvents)));
            CHECK_HR(_sensor->SetEventSink(sensorEvents));
        }

        std::vector<hid_sensor> wmf_hid_device::get_sensors()
        {
            std::vector<hid_sensor> sensors;

            HRESULT res = S_OK;
            BSTR fName{};
            LOG_HR(res = _sensor->GetFriendlyName(&fName));
            if (FAILED(res)) fName = L"Unidentified HID Sensor";

            sensors.push_back({ std::string(fName, fName + wcslen(fName)) });

            SysFreeString(fName);

            return sensors;
        }

        std::vector<uint8_t> wmf_hid_device::get_custom_report_data(const std::string & custom_sensor_name, const std::string & report_name, custom_sensor_report_field report_field)
        {
            return std::vector<uint8_t>();
        }

        void wmf_hid_device::foreach_hid_device(std::function<void(hid_device_info, CComPtr<ISensor>)> action)
        {
            try
            {
                CComPtr<ISensorManager> pSensorManager = nullptr;
                CComPtr<ISensorCollection> pSensorColl = nullptr;
                CComPtr<ISensor> pSensor = nullptr;
                ULONG uCount{};
                HRESULT res{};

                CComPtr<ISensorDataReport> ppDataReport;

                CHECK_HR(CoCreateInstance(CLSID_SensorManager, NULL,
                         CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(&pSensorManager)));

                LOG_HR(res=pSensorManager->GetSensorsByCategory(SENSOR_CATEGORY_ALL, &pSensorColl));
                if (SUCCEEDED(res))
                {
                    CHECK_HR(pSensorColl->GetCount(&uCount));

                    for (ULONG i = 0; i < uCount; i++)
                    {
                        if (SUCCEEDED(pSensorColl->GetAt(i, &pSensor.p)))
                        {
                            BSTR fName{};
                            LOG_HR(res = pSensor->GetFriendlyName(&fName));
                            if (FAILED(res)) fName= L"Unidentified HID sensor";

                            SENSOR_ID id{};
                            CHECK_HR(pSensor->GetID(&id));

                            SENSOR_TYPE_ID type{};
                            CHECK_HR(pSensor->GetType(&type));

                            CComPtr<IPortableDeviceValues> pValues = nullptr;  // Output

                            hid_device_info info{};

                            auto hr = pSensor->GetProperties(nullptr, &pValues);
                            DWORD cVals = 0; // Count of returned properties.
                            if (SUCCEEDED(hr))
                            {
                                // Get the number of values returned.
                                hr = pValues->GetCount(&cVals);
                                if (SUCCEEDED(hr))
                                {
                                    PROPERTYKEY pk; // Keys
                                    PROPVARIANT pv = {}; // Values

                                    // Loop through the values
                                    for (DWORD j = 0; j < cVals; j++)
                                    {
                                        // Get the value at the current index.
                                        hr = pValues->GetAt(j, &pk, &pv);
                                        if (SUCCEEDED(hr))
                                        {
                                            if (IsEqualPropertyKey(pk, SENSOR_PROPERTY_DEVICE_PATH))
                                            {
                                                info.device_path = std::string(pv.pwszVal, pv.pwszVal + wcslen(pv.pwszVal));
                                                info.id = std::string(fName, fName + wcslen(fName));

                                                uint16_t vid, pid, mi;
                                                std::string uid;
                                                if (parse_usb_path(vid, pid, mi, uid, info.device_path))
                                                {
                                                    info.unique_id = "*";
                                                    info.pid = to_string() << std::hex << pid;
                                                    info.vid = to_string() << std::hex << vid;
                                                }
                                            }

                                            //if (IsEqualPropertyKey(pk, SENSOR_PROPERTY_MODEL))
                                            //{
                                            //    info.pid = std::string(pv.pwszVal, pv.pwszVal + wcslen(pv.pwszVal));
                                            //}
                                        }
                                        PropVariantClear(&pv);
                                    }
                                }
                            }

                            action(info, pSensor);

                            //if (wcsstr(fName, L"Accelerometer") != NULL)
                            //if (wcsstr(fName, L"Gyroscope") != NULL)
                            //{
                            //    auto callback = new SensorEvents();
                            //    ISensorEvents* sensorEvents;
                            //    HRESULT hr = callback->QueryInterface(IID_PPV_ARGS(&sensorEvents));
                            //    hr = pSensor->SetEventSink(sensorEvents);
                            //    //for (int i = 0; i < 10000; i++)
                            //    //{
                            //    //    hr = pSensor->GetData(&ppDataReport);
                            //    //    if (ppDataReport != NULL)
                            //    //    {
                            //    //        SYSTEMTIME time;
                            //    //        ppDataReport->GetTimestamp(&time);

                            //    //        printf("%d.%d.%d %d:%d:%d.%d\n", time.wDay, time.wMonth, time.wYear, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);

                            //    //        IPortableDeviceValues* values;
                            //    //        ppDataReport->GetSensorValues(NULL, &values);

                            //    //        DWORD valuesCount = 0;
                            //    //        hr = values->GetCount(&valuesCount);
                            //    //        if (SUCCEEDED(hr))
                            //    //        {
                            //    //            PROPERTYKEY pk; // Keys
                            //    //            PROPVARIANT pv = {}; // Values

                            //    //            for (DWORD i = 0; i < valuesCount; i++)
                            //    //            {
                            //    //                // Get the value at the current index.
                            //    //                hr = values->GetAt(i, &pk, &pv);
                            //    //                if (SUCCEEDED(hr))
                            //    //                {
                            //    //                    if (IsEqualPropertyKey(pk, SENSOR_DATA_TYPE_CUSTOM_USAGE))
                            //    //                    {
                            //    //                        wprintf_s(L"\Acceleration X: %lu\n", pv.ulVal);
                            //    //                    }
                            //    //                }
                            //    //            }
                            //    //        }
                            //    //        //SENSOR_DATA_TYPE_ACCELERATION_X_G
                            //    //    }
                            //    //}
                            //    //printf("\n");
                            //}
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
