#pragma once

#include <chrono>
#include <queue>
#include <network_interaction/pdn_connection.h>

class SessionManager {
public:
    struct Session {
        std::string imsi;
        std::chrono::steady_clock::time_point expiry_time;
        std::shared_ptr<pdn_connection> connection;
    };

    void add_session(const std::string& imsi, std::shared_ptr<pdn_connection> conn, uint32_t timeout_sec) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.push({imsi,std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec),conn});
    }

    std::vector<Session> get_expired_sessions() {
        std::vector<Session> expired;
        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(mutex_);
        while (!sessions_.empty() && sessions_.top().expiry_time <= now) {
            expired.push_back(sessions_.top());
            sessions_.pop();
        }
        return expired;
    }

    std::vector<Session> get_sessions_for_shutdown(size_t count) {
        std::vector<Session> result;
        std::lock_guard<std::mutex> lock(mutex_);

        while (!sessions_.empty() && result.size() < count) {
            result.push_back(sessions_.top());
            sessions_.pop();
        }
        return result;
    }

private:
    struct CompareSession {
        bool operator()(const Session& a, const Session& b) const {
            return a.expiry_time > b.expiry_time;
        }
    };

    std::priority_queue<Session, std::vector<Session>, CompareSession> sessions_;
    std::mutex mutex_;
};
