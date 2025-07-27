# pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <mutex>

class BlacklistHandler {
public:
    explicit BlacklistHandler(const std::vector<std::string>& blacklist) : imsi_blacklist(blacklist) {
        std::sort(imsi_blacklist.begin(), imsi_blacklist.end());
    }

    bool is_blacklisted(const std::string& imsi) const {
        return std::binary_search(imsi_blacklist.begin(), imsi_blacklist.end(), imsi);
    }

    void update_blacklist(const std::vector<std::string>& new_blacklist) {
        std::lock_guard<std::mutex> lock(mutex);
        imsi_blacklist = new_blacklist;
        std::sort(imsi_blacklist.begin(), imsi_blacklist.end());
    }

private:
    std::vector<std::string> imsi_blacklist;
    mutable std::mutex mutex;
};