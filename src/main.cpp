#include <iostream>
#include "../include/server_data.h"
#include "../include/udp_server.h"

using std::cerr;
using std::endl;
using std::cout;

int main() {
    try {
        ServerConf conf("../pgw_server.json");

        const auto& config = conf.get_conf();

        UDPServer server(config);
        std::cout << "Сервер запущен " << config.udp_ip << ":" << config.udp_port << "\n";

        server.launch();

    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
