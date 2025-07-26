#include <gtest/gtest.h>
#include <pgw_core/server_data.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class Server_conf_test : public ::testing::Test {
protected:
    void SetUp() {
        test_config_path = "test_config.json";
        create_test_config();
    }

    void TearDown() {
        if (fs::exists(test_config_path)) {
            fs::remove(test_config_path);
        }
    }

    void create_test_config() {
        std::ofstream out(test_config_path);
        out << R"({
            "udp_ip": "127.0.0.1",
            "udp_port": 49155,
            "http_port": 49255,
            "cdr_file": "cdr_test.log",
            "log_file": "pgw_test.log",
            "log_level": "DEBUG",
            "session_timeout_sec": 12,
            "graceful_shutdown_rate": 5,
            "blacklist": [
            "001010123456789",
            "001010000000001"
            ]
        })";
        out.close();
    }

    std::string test_config_path;
};

// тест на загрузку конфига
TEST_F(Server_conf_test, loads_valid_config) {
    ServerConf conf(test_config_path);
    auto config = conf.get_conf();

    EXPECT_EQ(config.udp_ip.to_string(), "127.0.0.1");
    EXPECT_EQ(config.udp_port, 49155);
    EXPECT_EQ(config.http_port, 49255);
    EXPECT_EQ(config.cdr_log, "cdr_test.log");
    EXPECT_EQ(config.pgw_log_file, "pgw_test.log");
    EXPECT_EQ(config.log_level, "DEBUG");
    EXPECT_EQ(config.session_timeout_sec, 12);
    EXPECT_EQ(config.graceful_shutdown_rate, 5);
    EXPECT_EQ(config.imsi_blacklist.size(), 2);
}

// тест на отсутствующий файл
TEST_F(Server_conf_test, throwsonmissingfile) {
    EXPECT_THROW(
        ServerConf("non_existent.json"),
        std::runtime_error
    );
}

// тест невалидного JSON
TEST_F(Server_conf_test, throws_on_invalid_JSON) {
    std::ofstream out("bad_config.json");
    out << "{ invalid_json }";
    out.close();

    EXPECT_THROW(
        ServerConf("bad_config.json"),
        std::runtime_error
    );

    fs::remove("bad_config.json");
}

// тест на невалидный ip
TEST_F(Server_conf_test, throws_on_invalid_IP) {
    std::ofstream out("wrong.json");
    out << R"({"udp_ip": "34255.56"})";
    out.close();

    EXPECT_THROW(
        ServerConf("wrong.json"),
        std::runtime_error
    );

    fs::remove("wrong.json");
}

// тест валидации данных
TEST_F(Server_conf_test, check_data_validate) {
    ServerConf conf(test_config_path);
    auto config = conf.get_conf();

    EXPECT_NO_THROW(conf.check_data(config));

    auto bad_config = config;
    bad_config.udp_port = 0;
    EXPECT_THROW(conf.check_data(bad_config), std::runtime_error);
}
