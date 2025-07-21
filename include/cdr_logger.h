#pragma once
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <filesystem>

class CDRLogger {
public:
    explicit CDRLogger(const std::string& file_path) : cdr_file_path(file_path) {
        std::error_code e;
        std::filesystem::path path(file_path);

        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path(), e);
            if (e) {
                throw std::runtime_error(e.message());
            }
        }

        if (std::ofstream test_file(file_path, std::ios::app); !test_file) {
            throw std::runtime_error("Ошибка при открытии CDR файла");
        }
    }

    void log(const std::string& imsi, const std::string& action) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ofstream file(cdr_file_path, std::ios::app);
        if (file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            file << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
                 << "," << imsi << "," << action << "\n";
        }
    }

private:
    std::string cdr_file_path;
    std::mutex log_mutex;
};