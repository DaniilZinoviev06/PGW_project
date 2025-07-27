#pragma once

#include <iostream>
#include <atomic>
#include <bits/ostream.tcc>
#include <cstring>
#include <functional>
#include <string>
#include <thread>

// класс для слежки за конфигурационным файлом сервера
// в cpp файле все объяснение

class FileSurveillance {
public:
    FileSurveillance(const std::string& path, std::function<void()> cb)
        : file_path(path), callback(cb) {};

    ~FileSurveillance() { stop_monitoring(); };

    void monitoring();

    void stop_monitoring();

private:
    std::string file_path;
    int conf_fd = -1;
    int wd = -1;
    std::thread stalker;
    std::atomic<bool> on{true};
    std::function<void()> callback;
};