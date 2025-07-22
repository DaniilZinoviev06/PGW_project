#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include "../include/server_data.h"
#include "control_plane.h"
#include "http_api.h"
#include "session_handler.h"
#include <boost/asio/ip/address_v4.hpp>

class UDPServer {
public:
    explicit UDPServer(const server_configuration& config) :
        server_conf_data(config), cp(),
        session_handler(cp, config.imsi_blacklist, config.cdr_log, config.cdr_log, config.session_timeout_sec, 0.2)
    {
        cp.add_apn("default", boost::asio::ip::make_address_v4("192.168.0.1"));

        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            throw std::runtime_error("Ошибка создания epoll");
        }

        socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (socket_fd == -1) {
            throw std::runtime_error("Создание сокета прошло неудачно");
        }

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(server_conf_data.udp_ip.to_string().c_str());
        server_address.sin_port = htons(server_conf_data.udp_port);

        if (bind(socket_fd, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
            close(socket_fd);
            throw std::runtime_error("Привязка данных к сокету прошла неудачно");
        }

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = socket_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
            close(socket_fd);
            throw std::runtime_error("Ошибка добавления сокета в epoll");
        }

        http_server = std::make_unique<HTTPServer>(config, cp, session_handler);
        http_server->start();
    }

    void launch() {
        struct epoll_event events[10];

        std::cout << "HTTP сервер запущен | Порт - " << server_conf_data.http_port << std::endl;
        std::cout << "Сервер запущен\n";

        while (true) {
            int nfds = epoll_wait(epoll_fd, events, 10, -1);
            if (nfds == -1) {
                if (errno == EINTR)
                    continue;
                throw std::runtime_error("Ошибка epoll_wait");
            }

            for (int i = 0; i < nfds; ++i) {
                if (events[i].data.fd == socket_fd) {
                    handle_udp_event();
                }
            }
        }
    }

    ~UDPServer() {
        if (epoll_fd != -1)
            close(epoll_fd);
        if (socket_fd != -1)
            close(socket_fd);
    }

private:
    void handle_udp_event() {
        char buffer[256];
        sockaddr_in cliaddr{};
        socklen_t len = sizeof(cliaddr);

        while (true) {
            ssize_t n = recvfrom(socket_fd, buffer, sizeof(buffer), MSG_DONTWAIT, (sockaddr*)&cliaddr, &len);
            if (n <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                perror("recvfrom");
                break;
            }

            std::string imsi(buffer, n);
            std::string response = decode_imsi(imsi);
            sendto(socket_fd, response.c_str(), response.size(), 0, (sockaddr*)&cliaddr, len);
        }
    }

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

        if (session_handler.is_imsi_blacklisted(decode)) {
            session_handler.log_cdr(decode, "rejected");
            return "rejected\n";
        }

        if (auto httpserver = httpserver_.lock()) {
            httpserver->create_session(decode);
        }

        try {
            std::shared_ptr<pdn_connection> pdn = cp.create_pdn_connection("default",
                boost::asio::ip::make_address_v4("192.168.0.1"), 12345);
            pdn->set_imsi(decode);

            std::shared_ptr<bearer> bearer = cp.create_bearer(pdn, 54321);

            session_handler.log_cdr(decode, "created");
            return "created\n";

        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            session_handler.log_cdr(decode, "rejected");
            return "rejected\n";
        }
    }

    int epoll_fd = -1;
    int socket_fd = -1;
    const server_configuration& server_conf_data;
    control_plane cp;
    SessionHandler session_handler;
    std::unique_ptr<HTTPServer> http_server;
    std::weak_ptr<HTTPServer> httpserver_;
};