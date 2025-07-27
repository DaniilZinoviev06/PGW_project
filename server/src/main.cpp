#include <iostream>
#include "pgw_core/server_data.h"
#include "pgw_core/pgw_entrance_point.h"
#include <memory>

namespace {
    std::atomic<bool> global_shutdown_flag{false};
}

void signal_handler(int) {
    global_shutdown_flag.store(true);
}

int main() {
    try {
        {
            PGWEntrancePoint pgw("../../pgw_server.json");
            pgw.start_pgw();

            std::atomic<bool> running{true};
            std::thread timeout_thread([&]() {
                while (running) {
                    pgw.check_session_timeouts();
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });

            std::signal(SIGINT, signal_handler);
            std::signal(SIGTERM, signal_handler);

            while (!global_shutdown_flag.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            running = false;
            if (timeout_thread.joinable()) {
                timeout_thread.join();
            }
            pgw.graceful_shutdown();
        }

        INFOLogger::info("PGW successfully shut down");
    } catch (const std::exception& e) {
        INFOLogger::critical("Fatal error: {}", e.what());
        return 1;
    }
    return 0;
}