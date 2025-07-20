include(FetchContent)

#find_package(Boost 1.87 COMPONENTS asio)

find_package(nlohmann_json 3.12 QUIET)
if (NOT ${nlohmann_json_FOUND})
    FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif ()

find_package(Boost 1.87 QUIET COMPONENTS system)
if (NOT ${Boost_FOUND})
    FetchContent_Declare(
            boost_asio
            GIT_REPOSITORY https://github.com/boostorg/asio.git
            GIT_TAG boost-1.87.0
    )
    FetchContent_MakeAvailable(boost_asio)
endif ()

set(RUNNABLE ${CMAKE_PROJECT_NAME})
set(OBJ_LIB "${CMAKE_PROJECT_NAME}_lib")
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
add_library(${OBJ_LIB} OBJECT ${SOURCES})

target_include_directories(${OBJ_LIB} SYSTEM PUBLIC ${boost_asio_SOURCE_DIR}/include)
target_include_directories(${OBJ_LIB} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_options(${OBJ_LIB} PUBLIC "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-Wno-error=maybe-uninitialized")

add_executable(${RUNNABLE} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(${RUNNABLE} ${OBJ_LIB})
