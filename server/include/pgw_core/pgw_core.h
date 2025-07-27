#pragma once
#include <network_interaction/pdn_connection.h>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "../sessions/sessions_manager.h"
#include "../helpers/info_logger.h"

class PGWCore {
public:
    void add_session(const std::string& imsi, std::shared_ptr<pdn_connection> conn, uint32_t timeout_sec) {
        session_manager_.add_session(imsi, conn, timeout_sec);
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_[conn.get()] = conn;
    }

    ~PGWCore() {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.clear();
    }

    void remove_session(std::shared_ptr<pdn_connection> conn) {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        active_connections_.erase(conn.get());

        INFOLogger::debug("Сессия удалена");
    }

    std::vector<SessionManager::Session> get_sessions_to_close(int count) {
        auto expired = session_manager_.get_expired_sessions();
        if (!expired.empty()) {
            return expired;
        }

        return session_manager_.get_sessions_for_shutdown(count);
    }

    std::shared_ptr<pdn_connection> create_pdn_connection(
        const std::string& apn_name,
        const boost::asio::ip::address_v4& ue_address,
        uint32_t cp_teid)
    {
        std::lock_guard<std::mutex> lock(apn_mutex_);
        auto it = apn_list_.find(apn_name);
        if (it == apn_list_.end()) {
            throw std::runtime_error("APN не найден " + apn_name);
        }

        auto conn = pdn_connection::create(cp_teid, it->second, ue_address);
        return conn;
    }

    bool has_active_session(const std::string& imsi) const {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (const auto& [_, conn] : active_connections_) {
            if (conn->get_imsi() == imsi) {
                return true;
            }
        }
        return false;
    }

    size_t get_active_sessions_count() const {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        return active_connections_.size();
    }

    void add_apn(const std::string& name, const boost::asio::ip::address_v4& address) {
        std::lock_guard<std::mutex> lock(apn_mutex_);
        apn_list_[name] = address;
    }

private:
    SessionManager session_manager_;
    std::unordered_map<pdn_connection*, std::shared_ptr<pdn_connection>> active_connections_;
    std::unordered_map<std::string, boost::asio::ip::address_v4> apn_list_;
    mutable std::mutex connections_mutex_;
    mutable std::mutex apn_mutex_;
};