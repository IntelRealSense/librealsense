// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <httplib.h>

#include <librealsense2/rs.hpp>
#include <librealsense2-net/rs_net.hpp>

#include <fstream>
#include <iostream>

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

std::ostream &operator << ( std::ostream& strm, httplib::Error err )
{
    const std::string name_error[] = { 
        "Success",
        "Unknown",
        "Connection",
        "Bind IP Address",
        "Read",
        "Write",
        "Exceed Redirect Count",
        "Canceled",
        "SSL Connection",
        "SSL Loading Certs",
        "SSL Server Verification",
        "Unsupported Multipart Boundary Chars",
        "Compression",
    };
    return strm << name_error[err];
}


int main(int argc, char** argv) try {
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
        throw std::runtime_error("Specify either software or firmware update mode.");
    }

    std::string path = arg_sw.isSet() ? "/sw_upgrade" : "/fw_upgrade";

    std::string addr = arg_address.getValue();
    if (addr.empty()) {
        throw std::runtime_error("Server address is not specified.");
    }

    int port = 8080;
    if (arg_port.isSet()) {
        port = arg_port.getValue();
    } else {
        std::cout << "Using default port number." << std::endl;
    }

    std::string fname = arg_file.getValue();
    if (fname.empty()) {
        throw std::runtime_error("File name is not specified.");
        return EXIT_FAILURE;
    }
    std::cout << "Reading " << fname << std::endl;

    std::ifstream ifs(fname, std::ifstream::in | std::ifstream::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open the file.");
    }

    char byte;
    std::string fdata;
    while (ifs.get(byte)) fdata += byte;
    ifs.close();

    std::cout << "Sending " << fdata.size() << " bytes to " << addr << std::endl;
    httplib::Client client(addr, port);
    httplib::Result res = client.Post(path.c_str(), fdata, "application/octet-stream");
    if (res) {
        if (res->status == 200) {
            if (arg_fw.isSet()) {
                std::string status;
                std::string prev_status;
                time_t now, start = time(NULL);
                do {
                    res = client.Get("/fw_upgrade_status");
                    if (res) {
                        if (res->status == 200) {
                            status = res->body;
                            // Unite multiline status into the oneliner
                            std::string::size_type pos = 0;
                            while ((pos = status.find ("\n", pos)) != std::string::npos) {
                                status[pos] = ' ';
                            }
                            if (status.compare(prev_status)) {
                                prev_status = status;
                                std::cout << std::endl;
                            }
                            std::cout << status << ".                    \r" << std::flush;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    } else {
                        if (status.find("done") != std::string::npos) {
                            std::cout << std::endl;
                            return EXIT_SUCCESS;
                        } else throw std::runtime_error("Server closed the connection.");
                    }
                    now = time(NULL);
                } while (now - start < 300); // five minutes timeout
                throw std::runtime_error("Timeout while upgrating the firmware.");
            } else {
#define FIRST_TIMEOUT  (10)                
#define SECOND_TIMEOUT (5 * 60)                
                std::cout << "Waiting for the software upgrade to start." << std::flush;
                int seconds = 0;
                while (seconds++ < FIRST_TIMEOUT) {
                    httplib::Client client(addr, 8554);
                    httplib::Result res = client.Get("/");
                    if (res.error() == httplib::Error::Connection) break;
                    std::cout << "." << std::flush;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                if (seconds >= FIRST_TIMEOUT) {
                    throw std::runtime_error("Timeout while starting the software upgrade.");
                }

                std::cout << std::endl << "Software upgrade started." << std::flush;
                seconds = 0;
                while (seconds++ < SECOND_TIMEOUT) {
                    httplib::Client client(addr, 8554);
                    httplib::Result res = client.Get("/");
                    if (res.error() != httplib::Error::Connection) {
                        std::cout << std::endl << "Software upgrade is done." << std::endl;
                        break;
                    } 
                    std::cout << "." << std::flush;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                if (seconds >= SECOND_TIMEOUT) {
                    throw std::runtime_error("Timeout while upgrating the software.");
                }
            }
        } else throw std::runtime_error("Error server response: upgrade.");
    } else {
        std::stringstream s;
        s << res.error() << " error";
        throw std::runtime_error(s.str());
    }

    return EXIT_SUCCESS;
} catch (const std::exception& e) {
    std::cerr << std::endl << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
