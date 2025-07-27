#pragma once
#include <httplib.h>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include "../pgw_core/pgw_core.h"
#include "../sessions/session_handler.h"

class HTTPApi {
public:
    HTTPApi(uint16_t port, PGWCore& pgw_core, SessionHandler& session_handler,
        std::function<void()> shutdown_callback, const std::string& api_path = "/api/v1")
        : server(std::make_unique<httplib::Server>()), port(port), pgw_core_(pgw_core), session_handler_(session_handler),
          shutdown_callback_(shutdown_callback), api_path(api_path), is_shutting_down_(false) {}

    ~HTTPApi() {
        stop();
    }

    void start() {
        server_thread = std::thread([this]() {
            server->Get(api_path + "/check_imsi", [this](const httplib::Request& req, httplib::Response& res) {
                if (is_shutting_down_) {
                    res.status = 503;
                    res.set_content("Сервис недоступен - выполняется выключение", "text/plain");
                    return;
                }

                if (!req.has_param("imsi")) {
                    res.status = 400;
                    res.set_content("Необходим параметр IMSI", "text/plain");
                    return;
                }

                std::string imsi = req.get_param_value("imsi");
                bool is_active = pgw_core_.has_active_session(imsi);
                res.set_content(is_active ? "active\n" : "not active\n", "text/plain");
            });

            server->Get(api_path + "/stop", [this](const httplib::Request&, httplib::Response& res) {
                if (!is_shutting_down_.exchange(true)) {
                    res.set_content("Инициировано выключение", "text/plain");
                    shutdown_callback_();
                } else {
                    res.set_content("Выключение уже выполняется", "text/plain");
                }
            });

            server->listen("0.0.0.0", port);
        });
    }

    void stop() {
        if (!is_shutting_down_.exchange(true)) {
            if (server) {
                server->stop();
            }
            if (server_thread.joinable()) {
                server_thread.join();
            }
        }
    }

private:
    std::unique_ptr<httplib::Server> server;
    std::thread server_thread;
    uint16_t port;
    PGWCore& pgw_core_;
    SessionHandler& session_handler_;
    std::function<void()> shutdown_callback_;
    std::string api_path;
    std::atomic<bool> is_shutting_down_;
};