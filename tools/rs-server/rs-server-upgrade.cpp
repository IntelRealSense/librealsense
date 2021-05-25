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

    cmd.add(arg_address);
    cmd.add(arg_port);
    cmd.add(arg_file);

    cmd.parse(argc, argv);

    std::string addr = arg_address.getValue();

    int port = 8080;
    if (arg_port.isSet()) {
        port = arg_port.getValue();
    }

    std::string fname = arg_file.getValue();

    std::cout << "Reading " << fname << ".\n";

    char byte;
    std::string fdata;
    std::ifstream ifs(fname, std::ifstream::in | std::ifstream::binary);
    while (ifs.get(byte)) fdata += byte;
    ifs.close();

    std::cout << "Sending " << fdata.size() << " bytes to " << addr << ":" << port << ".\n";
    httplib::Client client(addr, port);
    client.Post("/sw_upgrade", fdata, "application/octet-stream");

    std::cout << "Done.\n";

    return 0;
}