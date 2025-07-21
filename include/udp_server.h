#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <memory>
#include "../include/server_data.h"
#include "session_handler.h"
#include "control_plane.h"

class UDPServer {
public:
    explicit UDPServer(const server_configuration& config) :
        server_conf_data(config), cp(),
        session_handler(cp, config.imsi_blacklist, config.cdr_log, config.cdr_log, config.session_timeout_sec, 0.2)
    {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd == -1) {
            throw std::runtime_error("Создание сокета прошло неудачно");
        }

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(server_conf_data.udp_ip.to_string().c_str());
        server_address.sin_port = htons(server_conf_data.udp_port);

        int socket_bind = bind(socket_fd, (sockaddr*)&server_address, sizeof(server_address));
        if (socket_bind == -1) {
            close(socket_fd);
            throw std::runtime_error("Привязка данных к сокету прошла неудачно");
        }
    }

    void launch() {
        char buffer[256];
        sockaddr_in cliaddr{};
        socklen_t len = sizeof(cliaddr);

        std::cout << "Сервер запущен\n";

        while (true) {
            ssize_t n = recvfrom(socket_fd, buffer, sizeof(buffer), 0, (sockaddr*)&cliaddr, &len);
            if (n > 0) {
                std::string imsi(buffer, n);
                std::string response = decode_imsi(imsi);
                sendto(socket_fd, response.c_str(), response.size(), 0, (sockaddr*)&cliaddr, len);
            }
        }
    }

    ~UDPServer() {
        if (socket_fd != -1)
            close(socket_fd);
    }

private:
    std::string decode_imsi(const std::string& tbcd_imsi) {
        std::string decode;

        if (tbcd_imsi.size() != 8) {
            std::cerr << "Некорректный IMSI\n";
            return "rejected";
        }

        for (uint8_t data_tbcd : tbcd_imsi) {
            uint8_t low_digit = data_tbcd & 0b00001111;
            uint8_t high_digit = data_tbcd >> 4;

            if (low_digit <= 9)
                decode += '0' + low_digit;
            if (high_digit <= 9)
                decode += '0' + high_digit;
        }

        if (!decode.empty() && decode.back() == 'F') {
            decode.pop_back();
        }

        // std::cout << "декодированный IMSI: " << decode << std::endl;

        if (session_handler.is_imsi_blacklisted(decode)) {
            session_handler.log_cdr(decode, "rejected");
            return "rejected";
        }

        session_handler.log_cdr(decode, "created");
        return "created";
    }

    int socket_fd = -1;
    const server_configuration& server_conf_data;
    control_plane cp;
    SessionHandler session_handler;
};