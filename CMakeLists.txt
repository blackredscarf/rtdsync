cmake_minimum_required(VERSION 3.13)
project(rtdsync)

set(CMAKE_CXX_STANDARD 17)
set(RTDCHAN_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_library(rtdsync include main.cpp)

target_include_directories(rtdsync
        PUBLIC
        ${RTDCHAN_INCLUDE_DIR}
        )