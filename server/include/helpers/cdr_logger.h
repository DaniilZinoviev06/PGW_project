#pragma once
#include <fstream>
#include <chrono>
#include <iomanip>

class CDRLogger {
public:
    explicit CDRLogger(const std::string& filename) : log_file(filename, std::ios::app) {
        if (!log_file.is_open()) {
            throw std::runtime_error("Ошибка при открытии cdr файла");
        }
    }

    void log(const std::string& imsi, const std::string& action) {
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);

        log_file << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S")
                << " | " << imsi << " | " << action << "\n";
        log_file.flush();
    }

private:
    std::ofstream log_file;
};