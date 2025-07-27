#pragma once
#include "../helpers/bloom_filter.h"
#include "../sessions/blacklist_handler.h"
#include "../helpers/cdr_logger.h"
#include <memory>
#include <mutex>
#include <unordered_set>
#include <cmath>
#include <vector>

class SessionHandler {
public:
    SessionHandler(const std::vector<std::string>& blacklist, const std::string& cdr_log_path,
        uint16_t session_timeout, double false_positive_prob = 0.1)
        : blacklist_handler(blacklist), cdr_logger(std::make_unique<CDRLogger>(cdr_log_path)), session_timeout(session_timeout),
        bloom_filter_size(calculate_bloom_size(blacklist.size(), false_positive_prob)),
        num_hash_funcs(calculate_hash_count(blacklist.size(), bloom_filter_size)),
        bloom_filter(BloomFilter(bloom_filter_size, num_hash_funcs))
    {
        for (const auto& imsi : blacklist) {
            bloom_filter.add(imsi);
        }
    }

    void log_session(const std::string& imsi, const std::string& action) {
        std::lock_guard<std::mutex> lock(mutex);
        cdr_logger->log(imsi, action);
    }

    bool is_imsi_blacklisted(const std::string& imsi) {
        std::lock_guard<std::mutex> lock(mutex);

        if (!bloom_filter.possibly_contains(imsi)) {
            return false;
        }

        return blacklist_handler.is_blacklisted(imsi);
    }

    void update_blacklist(const std::vector<std::string>& new_blacklist) {
        std::lock_guard<std::mutex> lock(mutex);
        blacklist_handler.update_blacklist(new_blacklist);
        bloom_filter_size = calculate_bloom_size(new_blacklist.size(), 0.1);
        num_hash_funcs = calculate_hash_count(new_blacklist.size(), bloom_filter_size);
        bloom_filter = BloomFilter(bloom_filter_size, num_hash_funcs);
        for (const auto& imsi : new_blacklist) {
            bloom_filter.add(imsi);
        }
    }

private:
    static size_t calculate_bloom_size(size_t n, double p) {
        return static_cast<size_t>(-(n * std::log(p)) / (std::log(2) * std::log(2)));
    }

    static size_t calculate_hash_count(size_t n, size_t m) {
        return static_cast<size_t>((m / n) * std::log(2));
    }

    BlacklistHandler blacklist_handler;
    std::unique_ptr<CDRLogger> cdr_logger;
    uint16_t session_timeout;
    size_t bloom_filter_size;
    size_t num_hash_funcs;
    BloomFilter bloom_filter;
    mutable std::mutex mutex;
};