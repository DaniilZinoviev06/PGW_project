#include "pgw_core/server_data.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
// #include <sys/inotify.h>
#include <set>

using json = nlohmann::json;

server_configuration ServerConf::load_data_from_json(std::string file_path) {
    server_configuration serv_conf;

    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Проблема открытия файла - " + file_path);
    }

    try {
        json config;
        file >> config;

        serv_conf.udp_ip = boost::asio::ip::make_address_v4(config["udp_ip"].get<std::string>());
        serv_conf.udp_port = config["udp_port"].get<uint16_t>();
        serv_conf.cdr_log = config["cdr_file"].get<std::string>();
        serv_conf.http_port = config["http_port"].get<uint16_t>();
        serv_conf.pgw_log_file = config["log_file"].get<std::string>();
        serv_conf.log_level = config["log_level"].get<std::string>();
        serv_conf.session_timeout_sec = config["session_timeout_sec"].get<uint16_t>();
        serv_conf.graceful_shutdown_rate = config["graceful_shutdown_rate"].get<uint16_t>();

        for (const auto& imsi : config["blacklist"]) {
            serv_conf.imsi_blacklist.push_back(imsi.get<std::string>());
        }
        //std::cout << "Конфигурация считана\n";
    } catch (const json::exception& e) {
        throw std::runtime_error("Ошибка при попытке распарсить json - " + std::string(e.what()));
    } catch (const boost::system::system_error& e) {
        throw std::runtime_error("Проверьте корректность ip адреса в json - " + std::string(e.what()));
    }

    return serv_conf;
}

void ServerConf::check_data(const server_configuration& config) {
    // Проверка портов
    if (config.udp_port < static_cast<uint16_t>(49152)) {
        throw std::runtime_error("Некорректный порт UDP");
    }

    if (config.http_port < static_cast<uint16_t>(49152)) {
        throw std::runtime_error("Некорректный порт http");
    }

    if (config.http_port == config.udp_port) {
        throw std::runtime_error("Порты должны различаться");
    }

    // Проверка IP-адреса
    if (config.udp_ip.is_unspecified()) {
        throw std::runtime_error("Некорректный IP-адрес");
    }

    if (config.cdr_log.empty()) {
        throw std::runtime_error("Пустой путь cdr_file");
    }

    if (config.pgw_log_file.empty()) {
        throw std::runtime_error("Пустой путь log_file");
    }

    if (config.log_level.empty()) {
        throw std::runtime_error("Пустой уровень логирования");
    }

    const std::set<std::string> valid_log_levels = {"DEBUG", "INFO", "WARN", "CRITICAL", "ERROR"};
    if (valid_log_levels.find(config.log_level) == valid_log_levels.end()) {
        throw std::runtime_error("Некорректный уровень логирования");
    }

    if (config.session_timeout_sec == 0) {
        throw std::runtime_error("Таймаут сессии не может быть равным 0");
    }

    if (config.graceful_shutdown_rate == 0) {
        throw std::runtime_error("Параметр graceful не может быть равным 0");
    }

    //std::cout << "Данные корректны\n";
}