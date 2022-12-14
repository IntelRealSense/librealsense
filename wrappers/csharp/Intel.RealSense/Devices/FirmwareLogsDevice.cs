// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    using System;
    using System.Runtime.InteropServices;

    public class FirmwareLogsDevice : Device
    {

        internal FirmwareLogsDevice(IntPtr dev)
            : base(dev)
        { }

        /// <summary>
        /// Create an <see cref="FirmwareLogsDevice"/> from existing <see cref="Device"/>
        /// </summary>
        /// <param name="dev">a device that supports <see cref="Extension.FirmwareLogger"/></param>
        /// <returns>a new <see cref="FirmwareLogsDevice"/></returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="dev"/> does not support <see cref="Extension.FirmwareLogger"/></exception>
        public static FirmwareLogsDevice FromDevice(Device dev)
        {
            object error;
            if (NativeMethods.rs2_is_device_extendable_to(dev.Handle, Extension.FirmwareLogger, out error) == 0)
            {
                throw new ArgumentException($"Device does not support {nameof(Extension.FirmwareLogger)}");
            }

            return Device.Create<FirmwareLogsDevice>(dev.Handle);
        }

        public FwLog CreateFwLog()
        {
            object error;
            return FwLog.Create(NativeMethods.rs2_create_fw_log_message(Handle, out error));
        }

        public FwParsedLog CreateFwParsedLog()
        {
            object error;
            return FwParsedLog.Create(NativeMethods.rs2_create_fw_log_parsed_message(Handle, out error));
        }

        public uint GetNumberOfFwLogs()
        {
            object error;
            return NativeMethods.rs2_get_number_of_fw_logs(Handle, out error);
        }

        public bool GetFwLog(ref FwLog fwLog)
        {
            object error;
            bool result = Convert.ToBoolean(NativeMethods.rs2_get_fw_log(Handle, fwLog.Handle, out error));
            return result;
        }

        public bool GetFlashLog(ref FwLog fwLog)
        {
            object error;
            bool result = Convert.ToBoolean(NativeMethods.rs2_get_flash_log(Handle, fwLog.Handle, out error));
            return result;
        }

        public bool InitParser(string xml_content)
        {
            object error;
            bool result = Convert.ToBoolean(NativeMethods.rs2_init_fw_log_parser(Handle, xml_content, out error));
            return result;
        }

        public bool ParseFwLog(FwLog fwLog, ref FwParsedLog fwParsedLog)
        {
            object error;
            bool result = Convert.ToBoolean(NativeMethods.rs2_parse_firmware_log(Handle, fwLog.Handle, fwParsedLog.Handle, out error));
            return result;
        }
    }
}
