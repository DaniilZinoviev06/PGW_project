#include <boost/asio/ip/address.hpp>
#include <iostream>
#include "../include/pdn_connection.h"
#include "../include/control_plane.h"
#include "../include/data_plane.h"
#include "../include/server_data.h"

using std::cerr;
using std::endl;
using std::cout;

int main() {
    try {
        server_configuration config = ServerConf::load_data_from_json("../pgw_server.json");

        std::cout << config.udp_ip << "\n"
            << config.udp_port << "\n"
            << config.http_port << "\n"
            << config.session_timeout_sec << "\n"
            << config.cdr_log << "\n"
            << config.log_level << "\n"
            << config.imsi_blacklist.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
