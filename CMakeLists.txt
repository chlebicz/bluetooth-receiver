cmake_minimum_required(VERSION 3.22)

message(STATUS "$ENV{IDF_PATH} test")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
project(bluetooth_receiver)
