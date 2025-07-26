cmake_minimum_required(VERSION 3.15)
project("simple_pgw" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

include(FetchContent)

find_package(nlohmann_json 3.12 QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(Boost 1.87 QUIET COMPONENTS system)
if(NOT Boost_FOUND)
    FetchContent_Declare(
            boost_asio
            GIT_REPOSITORY https://github.com/boostorg/asio.git
            GIT_TAG boost-1.87.0
    )
    FetchContent_MakeAvailable(boost_asio)
endif()

configure_file(
        "${CMAKE_SOURCE_DIR}/configuration/pgw_server.json"
        "${CMAKE_BINARY_DIR}/pgw_server.json"
        COPYONLY
)

<<<<<<< Updated upstream
add_subdirectory(src)
add_subdirectory(test)
=======
configure_file(
        "${CMAKE_SOURCE_DIR}/configuration/pgw_client.json"
        "${CMAKE_BINARY_DIR}/pgw_client.json"
        COPYONLY
)

add_subdirectory(server)
add_subdirectory(test)
add_subdirectory(client)
>>>>>>> Stashed changes
