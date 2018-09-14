using Intel.RealSense.Types;

namespace Intel.RealSense
{
    public class Log
    {
        public static void ToConsole(LogSeverity severity) 
            => NativeMethods.rs2_log_to_console(severity, out var err);

        public static void ToFile(LogSeverity severity, string filename) 
            => NativeMethods.rs2_log_to_file(severity, filename, out var err);
    }
}