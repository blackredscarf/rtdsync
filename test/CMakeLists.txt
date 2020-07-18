cmake_minimum_required(VERSION 3.13)
project(rtdsync_test)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        ${CMAKE_SOURCE_DIR}/../include
)

add_executable(test_chan test_chan.cpp)
add_executable(test_tick test_tick.cpp)
add_executable(test_waitgroup test_waitgroup.cpp)
add_executable(test_ringbuf test_ringbuf.cpp)

