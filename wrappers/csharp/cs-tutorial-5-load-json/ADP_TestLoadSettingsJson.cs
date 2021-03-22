using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace Intel.RealSense
{
    class ADP_TestLoadSettingsJson
    {
        public bool LoadSettingsJson(Device device)
        {
            var deviceName = device.Info[CameraInfo.Name];
            var usbType = device.Info[CameraInfo.UsbTypeDescriptor];
            string[] deviceNameParts = deviceName.Split(" ".ToCharArray());
            var model = deviceNameParts.Last().Trim();
            Console.WriteLine($"Device model: '{model}'");
            
            var jsonFileName = $"ADP_{model}_TEST_JSON_USB{usbType}.json";
            var jsonPath = System.IO.Path.Combine(System.IO.Directory.GetCurrentDirectory(), jsonFileName);
            Console.WriteLine($"Searching for a json test file: '{jsonFileName}'");
            if (!File.Exists(jsonPath))
            {
                Console.WriteLine($"File not found, so no settings data will be loaded into a camera. Exiting...");
                return false;
            }

            Console.WriteLine($"Json test file found: {jsonPath}. Loading it...");
            if (model == "L515")
            {
                Console.WriteLine($"Loading by using of SerialiazableDevice...");
                var ser = SerializableDevice.FromDevice(device);
                try
                {
                    ser.JsonConfiguration = File.ReadAllText(jsonPath);
                    Console.WriteLine($"Loaded");
                    Console.WriteLine(ser.JsonConfiguration);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to load json file: {ex.Message}");
                    return false;
                }
            }
            else
            {
                Console.WriteLine($"Loading by using of AdvancedDevice...");
                var adv = AdvancedDevice.FromDevice(device);
                try
                {
                    adv.JsonConfiguration = File.ReadAllText(jsonPath);
                    Console.WriteLine($"Loaded");
                    Console.WriteLine(adv.JsonConfiguration);
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Failed to load json file: {ex.Message}");
                    return false;
                }
            }
            return true;
        }
    }
}
