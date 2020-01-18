cmake_minimum_required(VERSION 3.13)
project(rtdsync)

set(CMAKE_CXX_STANDARD 17)
set(RTDSYNC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_library(rtdsync include main.cpp)

target_include_directories(rtdsync
        PUBLIC
        ${RTDSYNC_INCLUDE_DIR}
        )

include(GNUInstallDirs)

set(MYLIB_INSTALL_DIR ${PROJECT_SOURCE_DIR}/rtd)
IF (WIN32)
    set(CMAKE_INSTALL_PREFIX ${MYLIB_INSTALL_DIR})
ELSE()
    # set stuff for other systems
ENDIF()

set(MYLIB_INSTALL_INCLUDE_EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/)
install(DIRECTORY ${MYLIB_INSTALL_INCLUDE_EXPORT_DIR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS rtdsync
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR})