#include <bearer.h>

#include <pdn_connection.h>

bearer::bearer(uint32_t dp_teid, pdn_connection &pdn) : _dp_teid(dp_teid), _pdn(pdn) {}

uint32_t bearer::get_sgw_dp_teid() const {
    return _sgw_dp_teid;
}

void bearer::set_sgw_dp_teid(uint32_t sgw_cp_teid) {
    _sgw_dp_teid = sgw_cp_teid;
}

uint32_t bearer::get_dp_teid() const {
    return _dp_teid;
}

std::shared_ptr<pdn_connection> bearer::get_pdn_connection() const {
    return _pdn.shared_from_this();
}

void bearer::set_rate_limits(double uplink_mbps, double downlink_mbps) {
    uplink_limit = mbps_to_bytes(uplink_mbps);
    downlink_limit = mbps_to_bytes(downlink_mbps);
}

bool bearer::check_uplink_limit(size_t packet_size) {
    return check_impl(packet_size, uplink_used_data, uplink_limit, uplink_mutex);
}

bool bearer::check_downlink_limit(size_t packet_size) {
    return check_impl(packet_size, downlink_used_data, downlink_limit, downlink_mutex);
}

void bearer::reset_counters() {
    std::lock_guard<std::mutex> ulock(uplink_mutex);
    std::lock_guard<std::mutex> dlock(downlink_mutex);
    uplink_used_data = 0;
    downlink_used_data = 0;
    last_reset_time = std::chrono::steady_clock::now();
}

uint64_t bearer::mbps_to_bytes(double mbps) {
    return static_cast<uint64_t>(mbps * 125000);
}

bool bearer::check_impl(size_t size, uint64_t& used_data, uint64_t limit, std::mutex& mutex) {
    if (limit == 0)
        return true;

    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    if (now - last_reset_time >= std::chrono::seconds(1)) {
        reset_counters();
    }

    if (used_data + size > limit) {
        return false;
    }

    used_data += size;
    return true;
}
