#pragma once

#include <boost/asio/ip/address_v4.hpp>
#include <string>
#include <vector>

struct server_configuration {
    boost::asio::ip::address_v4 udp_ip;
    uint16_t udp_port;
    uint16_t http_port;
    uint16_t session_timeout_sec;
    uint16_t graceful_shutdown_rate;
    std::string cdr_log;
    std::string pgw_log_file;
    std::string log_level;
    std::vector<std::string> imsi_blacklist;
};

class ServerConf {
public:
    ServerConf(std::string file_path) : path(std::move(file_path)) {};

    ServerConf(const ServerConf&) = delete;
    ServerConf& operator=(const ServerConf&) = delete;

    ServerConf(ServerConf&&) = default;
    ServerConf& operator=(ServerConf&&) = default;

    static server_configuration load_data_from_json(std::string file_path);

    // void file_surveillance();

    void check_data(const server_configuration& config);

private:
    const std::string path;
};
