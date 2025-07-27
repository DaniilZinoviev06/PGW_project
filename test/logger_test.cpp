#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "../server/include/helpers/info_logger.h"

class logger_test : public ::testing::Test {
protected:
    void SetUp() override {
        file_test_log = "test.log";
        std::filesystem::remove(file_test_log);
    }

    void TearDown() override {
        std::filesystem::remove(file_test_log);
    }

    std::string file_test_log;
};

// тест на вывод логов при смене режимов
TEST_F(logger_test, level_change) {
    INFOLogger::init(file_test_log, "ERROR");

    testing::internal::CaptureStdout();
    INFOLogger::info("INFO не выведется");
    INFOLogger::error("ERROR выведется");

    INFOLogger::set_level("INFO");
    INFOLogger::info("отобр. INFO");

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output.find("INFO не выведется"), std::string::npos);
    EXPECT_NE(output.find("ERROR выведется"), std::string::npos);
    EXPECT_NE(output.find("отобр. INFO"), std::string::npos);
}
