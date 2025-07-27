#include <iostream>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

struct client_configuration {
    std::string server_ip;
    int server_port;
};

client_configuration load_config(const std::string& path) {
    std::ifstream f(path);
    json data = json::parse(f);
    return {
        data["server_ip"].get<std::string>(),
        data["server_port"].get<int>()
    };
}

std::vector<uint8_t> convert(const std::string& imsi) {
    std::vector<uint8_t> tbcd;
    tbcd.reserve((imsi.size() + 1) / 2);

    for (size_t i = 0; i < imsi.size(); i += 2) {
        uint8_t byte = imsi[i] - '0';

        if (i + 1 < imsi.size()) {
            byte |= (imsi[i+1] - '0') << 4;
        } else {
            byte |= 0b11110000;
        }

        tbcd.push_back(byte);
    }

    return tbcd;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Нет imsi\n";
        return 1;
    }

    std::string imsi_str = argv[1];
    if (imsi_str.size() != 15) {
        std::cerr << "imsi должен содержать 15 цифр\n";
        return 1;
    }

    try {
        auto config = load_config("../pgw_client.json");
        auto tbcd_imsi = convert(imsi_str);

        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in servaddr{};
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(config.server_port);
        inet_pton(AF_INET, config.server_ip.c_str(), &servaddr.sin_addr);

        std::string imsi(argv[1]);
        sendto(sockfd, tbcd_imsi.data(), tbcd_imsi.size(), 0, (const sockaddr*)&servaddr, sizeof(servaddr));

        char buffer[1024];
        socklen_t len = sizeof(servaddr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr*)&servaddr, &len);

        std::string response(buffer, n);
        std::cout << response << std::endl;

        close(sockfd);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}