cmake_minimum_required(VERSION 3.16)

# Temporary workaround - replace with your actual IDF path
set(IDF_PATH "C:/Users/lulas/esp/esp-idf")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(Netscore)
set(ESPTOOLPY_AFTER no_reset)