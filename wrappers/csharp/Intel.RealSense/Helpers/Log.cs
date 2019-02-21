
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
    }
}
