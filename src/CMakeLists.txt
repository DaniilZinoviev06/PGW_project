include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(RUNNABLE ${CMAKE_PROJECT_NAME})
set(OBJ_LIB "${CMAKE_PROJECT_NAME}_lib")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        #"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(${OBJ_LIB} OBJECT ${SOURCES})

target_include_directories(${OBJ_LIB} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${cpp-httplib_SOURCE_DIR}
        ${easyloggingpp_SOURCE_DIR}/src
)

target_include_directories(${OBJ_LIB} SYSTEM PUBLIC
        ${boost_asio_SOURCE_DIR}/include
        ${nlohmann_json_INCLUDE_DIRS}
        ${cpp-httplib_SOURCE_DIR}/include
)

target_include_directories(${OBJ_LIB} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_compile_options(${OBJ_LIB} PUBLIC "-Werror" "-Wall" "-Wextra" "-Wpedantic" "-Wno-error=maybe-uninitialized")

if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(${OBJ_LIB} PUBLIC nlohmann_json::nlohmann_json)
endif()

add_executable(${RUNNABLE} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(${RUNNABLE}
        ${OBJ_LIB}
        ${Boost_SYSTEM_LIBRARY}
        # cpp-httplib
)