#include <httplib.h>

#include <librealsense2/rs.hpp>
#include <librealsense2-net/rs_net.hpp>

#include <fstream>

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

int main(int argc, char** argv) {
    std::cout << "LRS-Net server software upgrade utility.\n";

    // Handle the command-line parameters
    TCLAP::CmdLine cmd("LRS-Net server software upgrade utility", ' ', RS2_API_VERSION_STR);
    TCLAP::ValueArg<std::string>  arg_address("a", "address", "Address of LRS-Net server", true, "", "string");
    TCLAP::ValueArg<unsigned int> arg_port("p", "port", "Port of LRS-Net server", false, 8554, "integer");
    TCLAP::ValueArg<std::string>  arg_file("f", "file", "File containing new package", true, "", "string");
    TCLAP::SwitchArg arg_sw("s", "sw", "Update the server software package");
    TCLAP::SwitchArg arg_fw("i", "fw", "Update the remote camera firmware image");

    cmd.add(arg_address);
    cmd.add(arg_port);
    cmd.add(arg_file);
    cmd.add(arg_sw);
    cmd.add(arg_fw);

    cmd.parse(argc, argv);

    if ( (!arg_sw.isSet() && !arg_fw.isSet()) || (arg_sw.isSet() && arg_fw.isSet()) ) {
        std::cout << "Please specify either software or firmware update." << std::endl;
        return EXIT_FAILURE;
    }

    std::string path = arg_sw.isSet() ? "/sw_upgrade" : "/fw_upgrade";

    std::string addr = arg_address.getValue();
    if (addr.empty()) {
        std::cout << "Please specify the server address." << std::endl;
        return EXIT_FAILURE;
    }

    int port = 8080;
    if (arg_port.isSet()) {
        port = arg_port.getValue();
    } else {
        std::cout << "Using default port number of 8554." << std::endl;
    }

    std::string fname = arg_file.getValue();
    if (fname.empty()) {
        std::cout << "Please specify the file name." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Reading " << fname << ".\n";

    std::ifstream ifs(fname, std::ifstream::in | std::ifstream::binary);
    if (!ifs.is_open()) {
        std::cout << "Cannot open the file." << std::endl;
        return EXIT_FAILURE;
    }

    char byte;
    std::string fdata;
    while (ifs.get(byte)) fdata += byte;
    ifs.close();

    std::cout << "Sending " << fdata.size() << " bytes to " << addr << std::endl;
    httplib::Client client(addr, port);
    httplib::Result res = client.Post(path.c_str(), fdata, "application/octet-stream");
    switch (res.error()) {
        case httplib::Error::Success:
            if (res->status == 200) {
                if (arg_fw.isSet()) {
                    while(true) {
                        res = client.Get("/fw_upgrade_status");
                        if (res) {
                            if (res->status == 200) {
                                // std::cout << res->body << std::endl;
                                printf("Firmware upgrade: %s          \r", res->body.c_str());
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            }
                        } else {
                            std::cout << "Firmware upgrade is done          " << std::endl;
                            break;
                        }
                    }
                } else {
                    std::cout << "Software upgrade is done" << std::endl;
                }
            } else {
                std::cout << "Failed: " << res->reason << std::endl;
                return EXIT_FAILURE;
            }
            break;
        case httplib::Error::Unknown:
            std::cout << "Unknown error" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::Connection:
            std::cout << "Connection error" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::BindIPAddress:
            std::cout << "Cannot bind" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::Read:
            std::cout << "Cannot read" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::Write:
            std::cout << "Cannot write" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::ExceedRedirectCount:
            std::cout << "Exceed redirect count" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::Canceled:
            std::cout << "Canceled" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::SSLConnection:
        case httplib::Error::SSLLoadingCerts:
        case httplib::Error::SSLServerVerification:
            std::cout << "SSL error" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::UnsupportedMultipartBoundaryChars:
            std::cout << "Unsupported multipart boundary charecters" << std::endl;
            return EXIT_FAILURE;
        case httplib::Error::Compression:
            std::cout << "Compression error" << std::endl;
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}