#pragma once
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <functional>
#include <memory>
#include "../pgw_core/pgw_core.h"
#include "../helpers/info_logger.h"

class UDPServer {
public:
    using MessageHandler = std::function<void(const std::string& imsi, const sockaddr_in& client_addr)>;

    UDPServer(boost::asio::ip::address_v4 ip, uint16_t port, PGWCore& core, SessionHandler& session_handler, uint32_t timeout_sec)
        : pgw_core(core), session_handler(session_handler), server_ip(ip), server_port(port), session_timeout_sec(timeout_sec) {

        create_socket();
        setup_epoll();
    }

    ~UDPServer() {
        stop();
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }

    void stop() {
        if (stop_requested.exchange(true)) {
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(server_ip.to_uint());
        server_addr.sin_port = htons(server_port);
        sendto(socket_fd, "", 0, 0, (sockaddr*)&server_addr, sizeof(server_addr));

        if (server_thread.joinable()) {
            server_thread.join();
        }

        if (epoll_fd != -1) {
            close(epoll_fd);
            epoll_fd = -1;
        }
        if (socket_fd != -1) {
            close(socket_fd);
            socket_fd = -1;
        }
    }

    void set_message_handler(MessageHandler handler) {
        message_handler = std::move(handler);
    }

    void run() {
        server_thread = std::thread([this]() {
            INFOLogger::info("UDP сервер запущен на {}:{}",
                server_ip.to_string(), server_port);

            epoll_event events[10];
            while (!stop_requested.load()) {
                int nfds = epoll_wait(epoll_fd, events, 10, 100); // Таймаут 100мс
                if (nfds == -1) {
                    if (errno == EINTR)
                        continue;
                    INFOLogger::error("ошибка epoll_wait: {}", strerror(errno));
                    break;
                }

                for (int i = 0; i < nfds; ++i) {
                    if (events[i].data.fd == socket_fd) {
                        handle_udp_message();
                    }
                }
            }

            INFOLogger::info("UDP сервер остановлен");
        });
    }

private:
    void create_socket() {
        socket_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (socket_fd == -1) {
            throw std::runtime_error("Ошибка создания сокета: " + std::string(strerror(errno)));
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(server_ip.to_uint());
        server_addr.sin_port = htons(server_port);

        if (bind(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr))) {
            close(socket_fd);
            throw std::runtime_error("Ошибка привязки сокета: " + std::string(strerror(errno)));
        }
    }

    void setup_epoll() {
        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            close(socket_fd);
            throw std::runtime_error("Ошибка создания epoll: " + std::string(strerror(errno)));
        }

        epoll_event event{};
        event.events = EPOLLIN;
        event.data.fd = socket_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event)) {
            close(socket_fd);
            close(epoll_fd);
            throw std::runtime_error("Ошибка epoll_ctl: " + std::string(strerror(errno)));
        }
    }

    void handle_udp_message() {
        char buffer[1024];
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        while (true) {
            ssize_t n = recvfrom(socket_fd, buffer, sizeof(buffer), MSG_DONTWAIT,
                                (sockaddr*)&client_addr, &addr_len);
            if (n <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                INFOLogger::error("Ошибка recvfrom: {}", strerror(errno));
                break;
            }

            std::string imsi(buffer, n);
            process_imsi(imsi, client_addr);
        }
    }

    void process_imsi(const std::string& tbcd_imsi, const sockaddr_in& client_addr) {
        std::string imsi = decode_imsi(tbcd_imsi);

        INFOLogger::debug("Обработка IMSI: {}", imsi);

        if (session_handler.is_imsi_blacklisted(imsi)) {
            INFOLogger::info("IMSI {} отклонен (в черном списке)", imsi);
            session_handler.log_session(imsi, "rejected_blacklist");
            send_response(client_addr, "rejected\n");
            return;
        }

        try {
            auto pdn = pgw_core.create_pdn_connection(
                "default",
                boost::asio::ip::address_v4(ntohl(client_addr.sin_addr.s_addr)),
                1234
            );
            pdn->set_imsi(imsi);

            pgw_core.add_session(imsi, pdn, session_timeout_sec);

            session_handler.log_session(imsi, "created");
            send_response(client_addr, "created\n");

            if (message_handler) {
                message_handler(imsi, client_addr);
            }
        } catch (const std::exception& e) {
            INFOLogger::error("Ошибка создания PDN: {}", e.what());
            session_handler.log_session(imsi, "rejected");
            send_response(client_addr, "rejected\n");
        }
    }

    std::string decode_imsi(const std::string& tbcd_imsi) {
        std::string imsi;
        for (uint8_t c : tbcd_imsi) {
            uint8_t low = c & 0x0F;
            uint8_t high = (c >> 4) & 0x0F;

            if (low <= 9) imsi += '0' + low;
            if (high <= 9) imsi += '0' + high;
            else if (high == 0x0F) break;
        }
        return imsi;
    }

    void send_response(const sockaddr_in& client_addr, const std::string& response) {
        sendto(socket_fd, response.data(), response.size(), 0,
              (const sockaddr*)&client_addr, sizeof(client_addr));
    }

    PGWCore& pgw_core;
    SessionHandler& session_handler;
    MessageHandler message_handler;
    boost::asio::ip::address_v4 server_ip;
    uint16_t server_port;
    int socket_fd = -1;
    int epoll_fd = -1;
    std::atomic<bool> stop_requested{false};
    std::thread server_thread;
    uint32_t session_timeout_sec;
};