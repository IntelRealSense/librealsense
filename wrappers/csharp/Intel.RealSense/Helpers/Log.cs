// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

namespace Intel.RealSense
{
    public static class Log
    {
        public static void ToConsole(LogSeverity severity)
        {
            object err;
            NativeMethods.rs2_log_to_console(severity, out err);
        }

        public static void ToFile(LogSeverity severity, string filename)
        {
            object err;
            NativeMethods.rs2_log_to_file(severity, filename, out err);
        }

        public static void LogMessage(LogSeverity severity, string message)
        {
            object err;
            NativeMethods.rs2_log(severity, message, out err);
        }
    }
}
