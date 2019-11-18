# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(alicloud_demo)

FILE(GLOB app_sources   src/*.c
                        src/eng/dev_sign/*.c
                        src/eng/infra/*.c
                        src/eng/mqtt/*.c
                        src/eng/wrappers/*.c)

# FILE(GLOB app_sources   src/*.c)

target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/eng/dev_sign)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/eng/infra)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/eng/mqtt)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/src/eng/wrappers)
