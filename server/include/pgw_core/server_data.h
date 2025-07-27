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
    ServerConf(const std::string& file_path) : data(load_data_from_json(file_path)) {
        check_data(data);
    }

    ServerConf(const ServerConf&) = delete;
    ServerConf& operator=(const ServerConf&) = delete;

    ServerConf(ServerConf&&) = default;
    ServerConf& operator=(ServerConf&&) = default;

    [[nodiscard]] const server_configuration& get_conf() const { return data; }

    void check_data(const server_configuration& config);

    server_configuration load_data_from_json(std::string file_path);

private:
    server_configuration data;
};
