#include "../uvc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sstream>

#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <linux/usb/video.h>

namespace rsimpl
{
    namespace uvc
    {
        struct context
        {
            // TODO: V4L2-specific information

            context() {} // TODO: Init
            ~context() {} // TODO: Cleanup
        };

        struct subdevice
        {
            // TODO: Stream-specific information
            std::string name;
            uint16_t camIndex;

            std::function<void(const void * frame)> callback;
        };

        struct device
        {
            const std::shared_ptr<context> parent;

            // TODO: Device-specific information
            std::vector<subdevice> subdevices;

            uint32_t serialNumber;

            device(std::shared_ptr<context> parent) : parent(parent) {} // TODO: Init
            ~device() {} // TODO: Cleanup
        };

        ////////////
        // device //
        ////////////

        int get_vendor_id(const device & device) { return 0; }
        int get_product_id(const device & device) { return 0; }

        void init_controls(device & device, int subdevice, const guid & xu_guid) {}
        void get_control(const device & device, int subdevice, uint8_t ctrl, void * data, int len) {}
        void set_control(device & device, int subdevice, uint8_t ctrl, void * data, int len) {}

        void claim_interface(device & device, const guid & interface_guid, int interface_number) {}
        void bulk_transfer(device & device, unsigned char endpoint, void * data, int length, int *actual_length, unsigned int timeout) {}

        void set_subdevice_mode(device & device, int subdevice_index, int width, int height, uint32_t fourcc, int fps, std::function<void(const void * frame)> callback) {}
        void start_streaming(device & device, int num_transfer_bufs) {}
        void stop_streaming(device & device) {}
        
        void set_pu_control(device & device, int subdevice, rs_option option, int value) {}
        int get_pu_control(const device & device, int subdevice, rs_option option) { return 0; }

        /////////////
        // context //
        /////////////

        std::shared_ptr<context> create_context()
        {
            return std::make_shared<context>();
        }

        std::vector<std::shared_ptr<device>> query_devices(std::shared_ptr<context> context)
        {

            std::vector<std::shared_ptr<device>> devices;

            auto make_subdevice = [&](char * name, uint16_t & indexDepth, uint16_t & indexIR, uint16_t & indexRGB)
            {
                auto subdev = std::make_shared<subdevice>();
                if (strcmp(name, "Intel(R) RealSense(TM) 3D Camera (R200) Depth") == 0)
                {
                    subdev.name = std::string(name);
                    subdev.camIndex = indexDepth++;
                }
                else if (strcmp(name, "Intel(R) RealSense(TM) 3D Camera (R200) Left-Right") == 0)
                {
                    subdev.name = std::string(name);
                    subdev.camIndex = indexIR++;
                }
                else if (strcmp(name, "Intel(R) RealSense(TM) 3D Camera (R200) RGB") == 0)
                {
                    subdev.name = std::string(name);
                    subdev.camIndex = indexRGB++;
                }
                return subdev;
            };

            try
            {
                std::stringstream ss;
                std::string strTmpDev;
                std::string strTmpInt;

                struct stat vstat;
                char buf[64] = {0};
                int fd = 0;
                int ret = 0;
                char * nl = NULL;

                uint16_t indexDepth = 0;
                uint16_t indexIR = 0;
                uint16_t indexRGB = 0;

                for (int i = 0; i < 64; ++i)
                {
                    ss.str("");
                    ss << "/dev/video" << i;

                    strTmpDev = ss.str();
                    if (stat(strTmpDev.c_str(), &vstat) < 0 || !S_ISCHR(vstat.st_mode))
                        break;

                    ss.str("");
                    ss << "/sys/class/video4linux/video" << i << "/device/interface";
                    strTmpInt = ss.str();

                    fd = open(strTmpInt.c_str(), O_RDONLY);
                    if (fd < 0)
                        continue;

                    ret = read(fd, buf, sizeof(buf));
                    close(fd);
                    if (ret < 0)
                        continue;

                    buf[63] = 0;

                    nl = strchr(buf, '\n');
                    if (!nl)
                        continue;

                    *nl = '\0';

                    if (strncmp(buf, "DS4", 3) == 0 || strncmp(buf, "Intel(R) RealSense(TM) 3D Camera (R200)", 39) == 0)
                    {
                        devices.subdevices.push_back(make_subdevice(buf, indexDepth, indexIR, indexRGB));
                    }
                }
            }
            catch (...)
            {
                throw std::runtime_error("failed to enumerate list of devices");
            }

            return devices;
        }
    }
}
