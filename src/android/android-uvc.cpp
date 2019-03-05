// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND


#include "android-uvc.h"
#include "../types.h"
#include "libuvc/utlist.h"
#include "usb_host/device_watcher.h"
#include "../libuvc/utlist.h"


#include <limits>


#define MAX_PINS 5
#define INTERRUPT_BUFFER_SIZE 0x400

namespace librealsense {
    namespace platform {
        // we are using standard fourcc codes to represent formats, while MF is using GUIDs
        // occasionally there is a mismatch between the code and the guid data
        const std::unordered_map<uint32_t, uint32_t> fourcc_map = {
                {0x59382020, 0x47524559},    /* 'GREY' from 'Y8  ' */
                {0x52573130, 0x70524141},    /* 'pRAA' from 'RW10'.*/
                {0x32000000, 0x47524559},    /* 'GREY' from 'L8  ' */
                {0x50000000, 0x5a313620},    /* 'Z16'  from 'D16 ' */
                {0x52415738, 0x47524559},    /* 'GREY' from 'RAW8' */
                {0x52573136, 0x42595232}     /* 'RW16' from 'BYR2' */
        };

        bool android_uvc_device::is_connected(const uvc_device_info &info) {
            auto result = true;

            //TODO: fix this
            return result;
        }

        void android_uvc_device::init_xu(const extension_unit &xu) {
            // not supported
            return;
        }

        bool android_uvc_device::set_xu(const extension_unit &xu, uint8_t ctrl, const uint8_t *data,
                                        int len) {

            int status = uvc_set_ctrl(_device.get(), xu.unit, ctrl, (void *) data, len);
            if (status < 0)
                LOG_ERROR("SetXU result:" << status);

            return status >= 0;
        }

        bool android_uvc_device::get_xu(const extension_unit &xu, uint8_t ctrl, uint8_t *data,
                                        int len) const {
            int status = uvc_get_ctrl(_device.get(), xu.unit, ctrl, (void *) data, len,
                                      UVC_GET_CUR);
            return status >= 0;
        }

        control_range
        android_uvc_device::get_xu_range(const extension_unit &xu, uint8_t control, int len) const {
            int status;
            int max;
            int min;
            int step;
            int def;
            status = uvc_get_ctrl(_device.get(), xu.unit, control, &max, sizeof(int32_t),
                                  UVC_GET_MAX);
            if (status < 0) {
                throw std::runtime_error(
                        to_string() << "uvc_get_ctrl(...) returned for UVC_GET_MAX "
                                    << status);
            }

            status = uvc_get_ctrl(_device.get(), xu.unit, control, &min, sizeof(int32_t),
                                  UVC_GET_MIN);
            if (status < 0) {
                throw std::runtime_error(
                        to_string() << "uvc_get_ctrl(...) returned for UVC_GET_MIN"
                                    << status);
            }

            status = uvc_get_ctrl(_device.get(), xu.unit, control, &step, sizeof(int32_t),
                                  UVC_GET_RES);
            if (status < 0) {
                throw std::runtime_error(
                        to_string() << "uvc_get_ctrl(...) returned for UVC_GET_RES"
                                    << status);
            }

            status = uvc_get_ctrl(_device.get(), xu.unit, control, &def, sizeof(int32_t),
                                  UVC_GET_DEF);
            if (status < 0) {
                throw std::runtime_error(
                        to_string() << "uvc_get_ctrl(...) returned for UVC_GET_DEF"
                                    << status);
            }

            control_range result(min, max, step, def);
            return result;
        }

        int android_uvc_device::rs2_option_to_ctrl_selector(rs2_option option, int &unit) const {
            // chances are that we will need to adjust some of these operation to the
            // processing unit and some to the camera terminal.
            unit = _processing_unit;
            switch (option) {
                case RS2_OPTION_BACKLIGHT_COMPENSATION:
                    return UVC_PU_BACKLIGHT_COMPENSATION_CONTROL;
                case RS2_OPTION_BRIGHTNESS:
                    return UVC_PU_BRIGHTNESS_CONTROL;
                case RS2_OPTION_CONTRAST:
                    return UVC_PU_CONTRAST_CONTROL;
                case RS2_OPTION_EXPOSURE:
                    unit = _input_terminal;
                    return UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;
                case RS2_OPTION_GAIN:
                    return UVC_PU_GAIN_CONTROL;
                case RS2_OPTION_GAMMA:
                    return UVC_PU_GAMMA_CONTROL;
                case RS2_OPTION_HUE:
                    return UVC_PU_HUE_CONTROL;
                case RS2_OPTION_SATURATION:
                    return UVC_PU_SATURATION_CONTROL;
                case RS2_OPTION_SHARPNESS:
                    return UVC_PU_SHARPNESS_CONTROL;
                case RS2_OPTION_WHITE_BALANCE:
                    return UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL;
                case RS2_OPTION_ENABLE_AUTO_EXPOSURE:
                    unit = _input_terminal;
                    return UVC_CT_AE_MODE_CONTROL; // Automatic gain/exposure control
                case RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE:
                    return UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL;
                case RS2_OPTION_POWER_LINE_FREQUENCY:
                    return UVC_PU_POWER_LINE_FREQUENCY_CONTROL;
                case RS2_OPTION_AUTO_EXPOSURE_PRIORITY:
                    unit = _input_terminal;
                    return UVC_CT_AE_PRIORITY_CONTROL;
                default:
                    throw linux_backend_exception(to_string() << "invalid option : " << option);
            }
        }

        // Retreive USB data, sign bit is calculated according to length
        int32_t android_uvc_device::get_data_usb(uvc_req_code action, int control, int unit,
                                                 unsigned int length) const {
            unsigned char buffer[4] = {0};
            int32_t ret = 0;

            int status = usb_device_control_transfer(
                    _device->device->get_handle(),
                    UVC_REQ_TYPE_GET,
                    action,
                    control << 8,
                    unit << 8 | (_device->deviceData.ctrl_if.bInterfaceNumber),
                    buffer,
                    sizeof(int32_t), 0);

            if (status < 0) {
                throw ("winusb_SendControl failed!");
            }

            if (status != sizeof(int32_t)) {
                throw ("insufficient data read from USB");
            }

            // Converting byte array buffer (with length 8/16/32) to int32
            switch (length) {
                case sizeof(uint8_t):
                    ret = B_TO_BYTE(buffer);
                    break;
                case sizeof(uint16_t) :
                    ret = SW_TO_SHORT(buffer);
                    break;
                case sizeof(uint32_t) :
                    ret = DW_TO_INT(buffer);
                    break;
                default:
                    throw std::runtime_error("unsupported length");
            }

            return ret;
        }

        void android_uvc_device::set_data_usb(uvc_req_code action, int control, int unit,
                                              int value) const {
            unsigned char buffer[4];

            INT_TO_DW(value, buffer);

            int status = usb_device_control_transfer(
                    _device->device->get_handle(),
                    UVC_REQ_TYPE_SET,
                    action,
                    control << 8,
                    unit << 8 | (_device->deviceData.ctrl_if.bInterfaceNumber),
                    buffer,
                    sizeof(int32_t), 0);

            if (status < 0) throw ("winusb_SendControl failed!");

            if (status != sizeof(int32_t))
                throw std::runtime_error("insufficient data writen to USB");
        }

        // Translate between UVC 1.5 Spec and RS
        int32_t android_uvc_device::rs2_value_translate(uvc_req_code action, rs2_option option,
                                                        int32_t value) const {
            // Value may be translated according to action/option value
            int32_t translated_value = value;

            switch (action) {
                case UVC_GET_CUR: // Translating from UVC 1.5 Spec up to RS
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        switch (value) {
                            case UVC_AE_MODE_D3_AP:
                                translated_value = 1;
                                break;
                            case UVC_AE_MODE_D0_MANUAL:
                                translated_value = 0;
                                break;
                            default:
                                throw std::runtime_error(
                                        "Unsupported GET value for RS2_OPTION_ENABLE_AUTO_EXPOSURE");
                        }
                    }
                    break;

                case UVC_SET_CUR: // Translating from RS down to UVC 1.5 Spec
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        switch (value) {
                            case 1:
                                // Enabling auto exposure
                                translated_value = UVC_AE_MODE_D3_AP;
                                break;
                            case 0:
                                // Disabling auto exposure
                                translated_value = UVC_AE_MODE_D0_MANUAL;
                                break;
                            default:
                                throw std::runtime_error(
                                        "Unsupported SET value for RS2_OPTION_ENABLE_AUTO_EXPOSURE");
                        }
                    }
                    break;

                case UVC_GET_MIN:
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        translated_value = 0; // Hardcoded MIN value
                    }
                    break;

                case UVC_GET_MAX:
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        translated_value = 1; // Hardcoded MAX value
                    }
                    break;

                case UVC_GET_RES:
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        translated_value = 1; // Hardcoded RES (step) value
                    }
                    break;

                case UVC_GET_DEF:
                    if (option == RS2_OPTION_ENABLE_AUTO_EXPOSURE) {
                        translated_value = 1; // Hardcoded DEF value
                    }
                    break;

                default:
                    throw std::runtime_error("Unsupported action translation");
            }
            return translated_value;
        }

        bool android_uvc_device::get_pu(rs2_option option, int32_t &value) const {
            int unit;
            int control = rs2_option_to_ctrl_selector(option, unit);
            unsigned int length = get_data_usb(UVC_GET_LEN, control, unit);

            value = get_data_usb(UVC_GET_CUR, control, unit, length);
            value = rs2_value_translate(UVC_GET_CUR, option, value);

            return true;
        }

        bool android_uvc_device::set_pu(rs2_option option, int32_t value) {
            int unit;
            int control = rs2_option_to_ctrl_selector(option, unit);

            value = rs2_value_translate(UVC_SET_CUR, option, value);
            set_data_usb(UVC_SET_CUR, control, unit, value);

            return true;
        }

        control_range android_uvc_device::get_pu_range(rs2_option option) const {
            int unit = 0;
            int min, max, step, def;
            int control = rs2_option_to_ctrl_selector(option, unit);

            min = get_data_usb(UVC_GET_MIN, control, unit);
            min = rs2_value_translate(UVC_GET_MIN, option, min);

            max = get_data_usb(UVC_GET_MAX, control, unit);
            max = rs2_value_translate(UVC_GET_MAX, option, max);

            step = get_data_usb(UVC_GET_RES, control, unit);
            step = rs2_value_translate(UVC_GET_RES, option, step);

            def = get_data_usb(UVC_GET_DEF, control, unit);
            def = rs2_value_translate(UVC_GET_DEF, option, def);

            control_range result(min, max, step, def);

            return result;
        }

        void android_uvc_device::set_power_state(power_state state) {
            std::lock_guard<std::mutex> lock(_power_mutex);

            if (state == D0 && _power_state == D3) {
                uint16_t vid = _info.vid, pid = _info.pid, mi = _info.mi;

                std::vector<std::shared_ptr<usbhost_uvc_device>> uvc_devices;
                for(auto&& dev : usb_host::device_watcher::get_device_list())
                {
                    std::shared_ptr<usbhost_uvc_device> uvc(new usbhost_uvc_device(),
                                                            [](usbhost_uvc_device *ptr) {usbhost_close(ptr); });
                    uvc->device = dev;
                    uvc->vid = dev->get_vid();
                    uvc->pid = dev->get_pid();
                    uvc_devices.push_back(uvc);
                }

                for (auto device : uvc_devices) {

                    usbhost_open(device.get(), mi);

                    if (device->deviceData.ctrl_if.bInterfaceNumber == mi) {
                        _device = device;

                        for (auto ct = _device->deviceData.ctrl_if.input_term_descs;
                             ct; ct = ct->next) {
                            _input_terminal = ct->bTerminalID;
                        }

                        for (auto pu = _device->deviceData.ctrl_if.processing_unit_descs;
                             pu; pu = pu->next) {
                            _processing_unit = pu->bUnitID;
                        }

                        for (auto eu = _device->deviceData.ctrl_if.extension_unit_descs;
                             eu; eu = eu->next) {
                            _extension_unit = eu->bUnitID;
                        }

                        _device->device->claim_interface(mi);

                        _power_state = D0;
                        return;
                    }
                }

                throw std::runtime_error("Device not found!");
            }
            if (state == D3 && _power_state == D0) {
                _device.reset();
                _power_state = D3;
            }
        }

        std::vector<stream_profile> android_uvc_device::get_profiles() const {
            std::vector<stream_profile> results;
            int i = 0;

            check_connection();

            if (get_power_state() != D0) {
                throw std::runtime_error("Device must be powered to query supported profiles!");
            }

            // Return linked list of uvc_format_t of all available formats inside device
            uvc_format_t *formats = NULL;
            usbhost_get_available_formats_all(_device.get(), &formats);

            uvc_format_t *curFormat = formats;
            while (curFormat != NULL) {
                stream_profile sp;
                sp.width = curFormat->width;
                sp.height = curFormat->height;
                sp.fps = curFormat->fps;
                sp.format = curFormat->fourcc;
                results.push_back(sp);
                curFormat = curFormat->next;
            }

            usbhost_free_formats(formats);

            return results;
        }


        //TODO: fix this
        android_uvc_device::android_uvc_device(const uvc_device_info &info,
                                               std::shared_ptr<const android_backend> backend)
                : _streamIndex(MAX_PINS), _info(info), _backend(std::move(backend)),
                  _location("")
        {
            if (!is_connected(info)) {
                throw std::runtime_error("Camera not connected!");
            }
        }

        android_uvc_device::~android_uvc_device() {
            try {
                android_uvc_device::set_power_state(D3);
            }
            catch (...) {
                // TODO: Log
            }
        }

        // profile config- set control
        void android_uvc_device::probe_and_commit(stream_profile profile, frame_callback callback,
                                                  int /*buffers*/) {
            if (_streaming)
                throw std::runtime_error("Device is already streaming!");

            _profiles.push_back(profile);
            _frame_callbacks.push_back(callback);
        }

        /* callback context send for each frame for its specific profile */
        struct callback_context {
            frame_callback _callback;
            stream_profile _profile;
            android_uvc_device *_this;
        };

        // receive the original callback and pass it to the right device. this is registered by libUVC.
        static void internal_winusb_uvc_callback(frame_object *frame, void *ptr) {
            callback_context *context = (callback_context *) ptr;
            android_uvc_device *device = context->_this;
            context->_callback(context->_profile, *frame, []() mutable {});
        }

        void android_uvc_device::play_profile(stream_profile profile, frame_callback callback) {
            bool foundFormat = false;

            // Return linked list of uvc_format_t of all available formats inside devices[0]
            uvc_format_t *formats = NULL;
            usbhost_get_available_formats_all(_device.get(), &formats);

            int i = 0;
            uvc_format_t *curFormat = formats;
            while (curFormat != NULL) {
                char *fourcc = (char *) &curFormat->fourcc;
                //printf("Format %d: Interface %d - FourCC = %c%c%c%c, width = %04d, height = %04d, fps = %03d\n", i, curFormat->interfaceNumber, fourcc[3], fourcc[2], fourcc[1], fourcc[0], curFormat->width, curFormat->height, curFormat->fps);
                curFormat = curFormat->next;
                i++;
            }

            /* Choose the first supported format for a given interface */
            DL_FOREACH(formats, curFormat) {
                if ((profile.format == curFormat->fourcc) &&
                    (profile.fps == curFormat->fps) &&
                    (profile.height == curFormat->height) &&
                    (profile.width == curFormat->width)) {
                    foundFormat = true;
                    break;
                }
            }

            if (foundFormat == false) {
                throw std::runtime_error("Failed to find supported format!");
            }

            if (update_stream_if_handle(_device.get(), curFormat->interfaceNumber) != UVC_SUCCESS) {
                throw std::runtime_error("Failed to find associated interface!");
            }

            uvc_stream_ctrl_t ctrl;
            usbhost_get_stream_ctrl_format_size(_device.get(), &ctrl, curFormat->fourcc,
                                                curFormat->width, curFormat->height, curFormat->fps,
                                                curFormat->interfaceNumber);

            usbhost_free_formats(formats);

            callback_context *context = new callback_context();
            context->_callback = callback;
            context->_this = this;
            context->_profile = profile;

            usbhost_start_streaming(_device.get(), &ctrl, internal_winusb_uvc_callback, context, 0);
        }

        void
        android_uvc_device::stream_on(std::function<void(const notification &n)> error_handler) {
            if (_profiles.empty())
                throw std::runtime_error("Stream not configured");

            if (_streaming)
                throw std::runtime_error("Device is already streaming!");

            check_connection();

            try {
                for (uint32_t i = 0; i < _profiles.size(); ++i) {
                    play_profile(_profiles[i], _frame_callbacks[i]);
                }

                _streaming = true;
            }
            catch (...) {
                for (auto &elem : _streams)
                    if (elem.callback)
                        close(elem.profile);

                _profiles.clear();
                _frame_callbacks.clear();

                throw;
            }
        }

        void android_uvc_device::start_callbacks() {
            _is_started = true;
        }

        void android_uvc_device::stop_callbacks() {
            _is_started = false;
        }

        void android_uvc_device::stop_stream_cleanup(const stream_profile &profile,
                                                     std::vector<profile_and_callback>::iterator &elem) {
            if (elem != _streams.end()) {
                elem->callback = nullptr;
                elem->profile.format = 0;
                elem->profile.fps = 0;
                elem->profile.width = 0;
                elem->profile.height = 0;
            }

            auto pos = std::find(_profiles.begin(), _profiles.end(), profile) - _profiles.begin();
            if (pos != _profiles.size()) {
                _profiles.erase(_profiles.begin() + pos);
                _frame_callbacks.erase(_frame_callbacks.begin() + pos);
            }

            if (_profiles.empty())
                _streaming = false;
        }

        void android_uvc_device::close(stream_profile profile) {
            _is_started = false;

            check_connection();

            usbhost_stop_streaming(_device.get());

            auto elem = std::find_if(_streams.begin(), _streams.end(), [&](const profile_and_callback &pac) {
                return (pac.profile == profile && (pac.callback));
            });

            if (elem == _streams.end() && _frame_callbacks.empty())
                throw std::runtime_error("Camera is not streaming!");

            if (elem != _streams.end()) {
                try {
                    flush(int(elem - _streams.begin()));
                }
                catch (...) {
                    stop_stream_cleanup(profile, elem); // TODO: move to RAII
                    throw;
                }
            }
            stop_stream_cleanup(profile, elem);
        }

        // ReSharper disable once CppMemberFunctionMayBeConst
        void android_uvc_device::flush(int sIndex) {
            if (is_connected(_info)) {

            }
        }

        void android_uvc_device::check_connection() const {
            if (!is_connected(_info))
                throw std::runtime_error("Camera is no longer connected!");
        }
    }
}

#endif
