add_executable(
        pgw_client
        pgw_client.cpp
)

target_link_libraries(
        pgw_client
        PRIVATE nlohmann_json::nlohmann_json
)