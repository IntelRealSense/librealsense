#include "../uvc.h"

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

            std::function<void(const void * frame)> callback;
        };

        struct device
        {
            const std::shared_ptr<context> parent;
            // TODO: Device-specific information
            std::vector<subdevice> subdevices;

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
            // TODO: Enumerate devices
            return devices;
        }
    }
}
