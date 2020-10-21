# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

add_compile_options (-Werror -fdiagnostics-color=always)

list(APPEND BOARD_ROOT $ENV{ZEPHYR_BASE}/../custom)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(beacon)

target_sources(app PRIVATE src/main.c src/temp_sensor_mcp9808.c src/voltage_sensor.c)
