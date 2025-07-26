set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(pgw_headers INTERFACE)
target_include_directories(pgw_headers INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_subdirectory(src)