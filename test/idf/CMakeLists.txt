# Build check for the ESP-IDF component: idf.py build (CI runs it for esp32).
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tinymenu_idf_check)
