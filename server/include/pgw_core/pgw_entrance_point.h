#pragma once
#include "../sessions/session_handler.h"
#include "../network_interaction/http_api.h"
#include "../network_interaction/udp_server.h"
#include "server_data.h"
#include "helpers/info_logger.h"
#include "helpers/file_surveillance.h"
#include <memory>
#include <atomic>

using namespace std::chrono_literals;

class PGWEntrancePoint {
public:
    explicit PGWEntrancePoint(const std::string& config_path)
        : config_path(config_path),
        config(std::make_unique<ServerConf>(config_path)) {

        INFOLogger::init(config->get_conf().pgw_log_file, config->get_conf().log_level);
        INFOLogger::info("Инициализация PGW: {}", config_path);

        setup_monitoring();

        pgw_core.add_apn("default", boost::asio::ip::make_address_v4("8.8.8.8"));
        session_handler = std::make_unique<SessionHandler>(
            config->get_conf().imsi_blacklist, config->get_conf().cdr_log,
            config->get_conf().session_timeout_sec, 0.1);

        udp_server = std::make_unique<UDPServer>(
            config->get_conf().udp_ip,
            config->get_conf().udp_port,
            pgw_core,
            *session_handler,
            config->get_conf().session_timeout_sec
        );

        http_api = std::make_unique<HTTPApi>(
            config->get_conf().http_port,
            pgw_core,
            *session_handler,
            [this]() { this->graceful_shutdown(); }
        );
    }

    void start_pgw() {
        INFOLogger::info("Запуск сервисов PGW");
        start_monitoring();
        udp_server->run();
        http_api->start();

        INFOLogger::info("Все сервисы PGW успешно запущены");
    }

    void graceful_shutdown() {
        if (shutdown_initiated.exchange(true)) {
            return;
        }

        INFOLogger::info("Начало плавного завершения работы",
            config->get_conf().graceful_shutdown_rate);

        INFOLogger::info("Остановка HTTP API");
        http_api->stop();

        INFOLogger::info("Остановка UDP сервера");
        udp_server->stop();

        INFOLogger::info("Остановка мониторинга");
        stop_monitoring();

        int total_sessions = pgw_core.get_active_sessions_count();
        INFOLogger::info("Всего сессий для закрытия: {}", total_sessions);

        int closed_sessions = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (closed_sessions < total_sessions) {
            auto batch_start = std::chrono::steady_clock::now();

            auto sessions = pgw_core.get_sessions_to_close(
                config->get_conf().graceful_shutdown_rate);

            for (const auto& session : sessions) {
                try {
                    session_handler->log_session(session.imsi, "graceful_shutdown");
                    pgw_core.remove_session(session.connection);
                    closed_sessions++;
                    INFOLogger::debug("Сессия закрыта IMSI: {}", session.imsi);
                } catch (const std::exception& e) {
                    INFOLogger::error("Ошибка при закрытии сессии {}: {}", session.imsi, e.what());
                }
            }

            if (!sessions.empty()) {
                float progress = (float)closed_sessions / total_sessions * 100;
                INFOLogger::info("Прогресс: {:.1f}% ({} из {})",
                               progress, closed_sessions, total_sessions);
            }

            auto batch_time = std::chrono::steady_clock::now() - batch_start;
            auto remaining = std::chrono::milliseconds(1000) - batch_time;
            if (remaining > std::chrono::milliseconds(0)) {
                std::this_thread::sleep_for(remaining);
            }
        }

        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        );

        INFOLogger::info("Завершение работы приложения заняло {} мс", total_time.count());
        std::exit(0);
    }

    void stop() {
        INFOLogger::info("Остановка сервисов PGW");

        udp_server->stop();
        http_api->stop();
        stop_monitoring();

        INFOLogger::info("Сервисы PGW остановлены");
    }

    void check_session_timeouts() {
        auto expired_sessions = pgw_core.get_sessions_to_close(0);

        for (const auto& session : expired_sessions) {
            try {
                session_handler->log_session(session.imsi, "timeout_expired");
                pgw_core.remove_session(session.connection);
                INFOLogger::info("Сессия истекла для IMSI: {}", session.imsi);
            } catch (const std::exception& e) {
                INFOLogger::error("Ошибка закрытия истекшей сессии {}: {}", session.imsi, e.what());
            }
        }
    }

private:
    void setup_monitoring() {
        try {
            file_monitor = std::make_unique<FileSurveillance>(
                config_path,
                [this]() {
                    INFOLogger::info("Обнаружено изменение конфигурационного файла!");
                    this->on_config_changed();
                });
            INFOLogger::info("Мониторинг конфигурации инициализирован");
        } catch (const std::exception& e) {
            INFOLogger::error("Ошибка инициализации мониторинга: {}", e.what());
        }
    }

    void start_monitoring() {
        if (file_monitor) {
            INFOLogger::debug("Запуск мониторинга конфигурации");
            file_monitor->monitoring();
            INFOLogger::info("Мониторинг конфигурации активен");
        }
    }

    void stop_monitoring() {
        if (file_monitor) {
            file_monitor->stop_monitoring();
            INFOLogger::info("Мониторинг конфигурации остановлен");
        }
    }

    void on_config_changed() {
        try {
            INFOLogger::info("Обнаружено изменение конфигурации, выполняется перезагрузка...");
            auto new_config = std::make_unique<ServerConf>(config_path);

            if (new_config->get_conf().log_level != config->get_conf().log_level) {
                INFOLogger::set_level(new_config->get_conf().log_level);
                INFOLogger::info("Уровень логирования изменен на: {}",
                    new_config->get_conf().log_level);
            }

            if (new_config->get_conf().imsi_blacklist != config->get_conf().imsi_blacklist) {
                session_handler->update_blacklist(new_config->get_conf().imsi_blacklist);
                INFOLogger::info("Черный список обновлен, теперь содержит {} IMSI",
                    new_config->get_conf().imsi_blacklist.size());
            }

            config = std::move(new_config);
            INFOLogger::info("Конфигурация успешно перезагружена");
        } catch (const std::exception& e) {
            INFOLogger::error("Ошибка при перезагрузке конфигурации: {}", e.what());
        }
    }

    const std::string config_path;
    std::unique_ptr<ServerConf> config;
    PGWCore pgw_core;
    std::unique_ptr<SessionHandler> session_handler;
    std::unique_ptr<HTTPApi> http_api;
    std::unique_ptr<UDPServer> udp_server;
    std::unique_ptr<FileSurveillance> file_monitor;
    std::atomic<bool> shutdown_initiated{false};
};