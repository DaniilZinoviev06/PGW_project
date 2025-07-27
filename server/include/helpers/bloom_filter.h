#pragma once
#include <vector>
#include <string>
#include <functional>
#include <unordered_set>

class BloomFilter {
public:
    BloomFilter(size_t size, size_t num_hashes)
        : filter_size(size), num_hashes(num_hashes), bit_array(size, false) {}

    void add(const std::string& item) {
        for (size_t i = 0; i < num_hashes; ++i) {
            size_t hash = hash_value(item, i) % filter_size;
            bit_array[hash] = true;
        }
    }

    [[nodiscard]] bool possibly_contains(const std::string& item) const {
        for (size_t i = 0; i < num_hashes; ++i) {
            size_t hash = hash_value(item, i) % filter_size;
            if (!bit_array[hash]) return false;
        }
        return true;
    }

private:
    [[nodiscard]] size_t hash_value(const std::string& item, size_t seed) const {
        return std::hash<std::string>{}(item + std::to_string(seed));
    }

    size_t filter_size;
    size_t num_hashes;
    std::vector<bool> bit_array;
};