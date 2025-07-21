#pragma once

#include "control_plane.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

class SessionHandler {
public:
    SessionHandler(control_plane& cp_ref,
        const std::vector<std::string>& blacklist, const std::string& cdr_log_path, uint16_t session_timeout, double false_positive_prob = 0.3)
        : cp(cp_ref), session_timeout(session_timeout), cdr_log_file(cdr_log_path),
            bloom_filter_size(calculate_bloom_size(blacklist.size(), false_positive_prob)),
            num_hash_funcs(calculate_hash_count(blacklist.size(), bloom_filter_size)),
            bloom_filter(bloom_filter_size), imsi_blacklist(blacklist)
    {
        std::sort(imsi_blacklist.begin(), imsi_blacklist.end());
        bloom_filter_init(blacklist);

        /*std::cout << bloom_filter_size << "\n"
            << num_hash_funcs << "\n"
            << false_positive_prob * 100 << "\n";*/
    }

    bool is_imsi_blacklisted(const std::string& imsi) {
        if (!bloom_filter_check(imsi)) {
            return false;
        }
        return std::binary_search(imsi_blacklist.begin(), imsi_blacklist.end(), imsi);
    }

private:
    control_plane& cp;
    uint16_t session_timeout;
    std::string cdr_log_file;
    size_t bloom_filter_size;
    size_t num_hash_funcs;
    std::vector<bool> bloom_filter{};
    std::vector<std::string> imsi_blacklist{};

    static size_t calculate_bloom_size(size_t n, double p) {
        return static_cast<size_t>(-(n * log(p)) / (log(2) * log(2)));
    }

    static size_t calculate_hash_count(size_t n, size_t m) {
        return static_cast<size_t>((m / n) * log(2));
    }

    void bloom_filter_init(const std::vector<std::string>& blacklist) {
        for (const std::string& imsi : blacklist) {
            for (size_t i = 0; i < num_hash_funcs; ++i) {
                std::string salted = imsi + std::to_string(i);
                size_t hash = std::hash<std::string>{}(salted) % bloom_filter_size;
                bloom_filter[hash] = true;
            }
        }
    }

    bool bloom_filter_check(const std::string& imsi) const {
        for (size_t i = 0; i < num_hash_funcs; ++i) {
            std::string salted = imsi + std::to_string(i);
            size_t hash = std::hash<std::string>{}(salted) % bloom_filter_size;
            if (!bloom_filter[hash]) {
                return false;
            }
        }
        return true;
    }
};