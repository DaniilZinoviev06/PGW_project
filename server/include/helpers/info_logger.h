#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <iostream>

// класс логгер. который ведет запись в файл и в терминал

class INFOLogger {
public:
    explicit INFOLogger(const std::string& file_path) : log_file_path(file_path) {
        std::error_code e;
        std::filesystem::path path(file_path);

        if (!path.parent_path().empty()) {
            std::filesystem::create_directories(path.parent_path(), e);
            if (e) {
                throw std::runtime_error(e.message());
            }
        }

        if (std::ofstream test_file(file_path, std::ios::app); !test_file) {
            throw std::runtime_error("Ошибка при открытии log файла");
        }
    };

    static void init(const std::string& log_file, const std::string& log_level) {
        try {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file);

            const char * pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
            const char * console_pattern = "%^[%Y-%m-%d %H:%M:%S.%e] [%l]%$ %v";
            console_sink->set_pattern(console_pattern);
            file_sink->set_pattern(pattern);

            auto logger = std::make_shared<spdlog::logger>("logger",
                spdlog::sinks_init_list{console_sink, file_sink});

            spdlog::level::level_enum level;
            if (log_level == "TRACE") level = spdlog::level::trace;
            else if (log_level == "DEBUG") level = spdlog::level::debug;
            else if (log_level == "INFO") level = spdlog::level::info;
            else if (log_level == "WARN") level = spdlog::level::warn;
            else if (log_level == "ERROR") level = spdlog::level::err;
            else if (log_level == "CRITICAL") level = spdlog::level::critical;
            else {
                std::cerr << "Некорректный уровень логирования. Установлен по умолчанию - DEBUG" << std::endl;
                level = spdlog::level::debug;
            }
            logger->set_level(level);
            logger->flush_on(level);

            spdlog::set_default_logger(logger);

            spdlog::info("Уровень: {}", log_level);
            logger->flush();

        }
        catch (const std::exception& ex) {
            std::cerr << ex.what() << std::endl;
            throw;
        }
    }

    static void set_level(const std::string& log_level) {
        if (log_level == "DEBUG") {
            spdlog::set_level(spdlog::level::debug);
        } else if (log_level == "INFO") {
            spdlog::set_level(spdlog::level::info);
        } else if (log_level == "WARN") {
            spdlog::set_level(spdlog::level::warn);
        } else if (log_level == "CRITICAL") {
            spdlog::set_level(spdlog::level::critical);
        } else if (log_level == "ERROR") {
            spdlog::set_level(spdlog::level::err);
        } else {
            throw std::runtime_error("Неизвестный уровень логирования: " + log_level);
        }
    }

    template<typename... Args>
    static void log(spdlog::level::level_enum level, fmt::format_string<Args...> fmt, Args&&... args) {
        spdlog::log(level, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(fmt::format_string<Args...> fmt, Args&&... args) {
        log(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(fmt::format_string<Args...> fmt, Args&&... args) {
        log(spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        log(spdlog::level::critical, fmt, std::forward<Args>(args)...);
    }

private:
    std::string log_file_path;
};
