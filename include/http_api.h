#pragma once

#include <httplib.h>
#include <atomic>
#include <thread>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include "../include/server_data.h"
#include "control_plane.h"
#include "session_handler.h"

class HTTPServer {
public:
    HTTPServer(const server_configuration& config, control_plane& c_plane, SessionHandler& session_handler)
        : conf(config), c_plane(c_plane), session_hr(session_handler) {}

    ~HTTPServer() {
        stop();
    }

    void start() {
        if (run)
            return;

        server_ = std::make_unique<httplib::Server>();
        setup_routes();

        run = true;
        serv_thr = std::thread([this]() {
            server_->listen("0.0.0.0", conf.http_port);
        });

    }

    void create_session(const std::string& imsi) {
        std::lock_guard<std::mutex> lock(sessions_mtx);
        active_sessions_map[imsi] = {std::chrono::steady_clock::now()};
    }

    void stop() {
        if (!run) return;

        shutdown_req = true;
        server_->stop();
        if (serv_thr.joinable()) {
            serv_thr.join();
        }
        if (ses_timer_thr.joinable()) {
            ses_timer_thr.join();
        }
        run = false;
    }

    bool is_running() const {
        return run;
    }

private:
    void setup_routes() {
        server_->Get("/check_subscriber", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("imsi")) {
                res.status = 400;
                res.set_content("Не указан imsi", "text/plain");
                return;
            }

            std::string imsi = req.get_param_value("imsi");
            bool is_active = c_plane.has_active_session(imsi);

            res.set_content(is_active ? "active" : "not active", "text/plain");
        });

    }

    struct SessionInfo {
        std::chrono::steady_clock::time_point last_activity;
    };

    const server_configuration& conf;
    control_plane& c_plane;
    SessionHandler& session_hr;
    std::unique_ptr<httplib::Server> server_;
    std::thread serv_thr;
    std::thread ses_timer_thr;
    std::atomic<bool> run{false};
    std::atomic<bool> shutdown_req{false};

    std::unordered_map<std::string, SessionInfo> active_sessions_map;
    std::mutex sessions_mtx;
};