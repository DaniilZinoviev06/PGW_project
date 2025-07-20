#pragma once

#include <boost/asio/ip/address_v4.hpp>

// здесь добавил лимиты

class pdn_connection;

class bearer {
public:
    bearer(uint32_t dp_teid, pdn_connection &pdn);

    [[nodiscard]] uint32_t get_sgw_dp_teid() const;
    void set_sgw_dp_teid(uint32_t sgw_cp_teid);

    [[nodiscard]] uint32_t get_dp_teid() const;

    [[nodiscard]] std::shared_ptr<pdn_connection> get_pdn_connection() const;

    void set_rate_limits(double uplink_mbps, double downlink_mbps);

    bool check_uplink_limit(size_t packet_size);

    bool check_downlink_limit(size_t packet_size);

    void reset_counters();

private:
    uint32_t _sgw_dp_teid{};
    uint32_t _dp_teid{};
    pdn_connection &_pdn;

    uint64_t uplink_limit = 0;
    uint64_t downlink_limit = 0;
    uint64_t uplink_used_data = 0;
    uint64_t downlink_used_data = 0;

    std::mutex uplink_mutex;
    std::mutex downlink_mutex;
    std::chrono::steady_clock::time_point last_reset_time = std::chrono::steady_clock::now();

    static uint64_t mbps_to_bytes(double mbps);

    bool check_impl(size_t size, uint64_t& used, uint64_t limit, std::mutex& mutex);
};
