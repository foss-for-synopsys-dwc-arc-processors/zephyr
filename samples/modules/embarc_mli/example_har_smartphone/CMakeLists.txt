# Copyright (c) 2022 Synopsys
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

zephyr_compile_definitions(
  MODEL_BIT_DEPTH=16
  V2DSP_XY
  _C_ARRAY_
)

zephyr_compile_options("-Wno-ignored-qualifiers")

zephyr_library_include_directories(
  ../auxiliary/
  src/
)

target_sources(app PRIVATE
    src/ml_api_har_smartphone_main.c
    src/har_smartphone_coefficients_fx.c
    src/har_smartphone_coefficients_sa.c
    src/har_smartphone_constants.h
    src/har_smartphone_model_sa.c
    src/har_smartphone_model_fx.c
    src/har_smartphone_model.h
    src/har_smartphone_ref_inout.h
    ../auxiliary/examples_aux.c
    ../auxiliary/examples_aux.h
    ../auxiliary/idx_file.c
    ../auxiliary/idx_file.h
    ../auxiliary/tensor_transform.c
    ../auxiliary/tensor_transform.h
    ../auxiliary/tests_aux.c
    ../auxiliary/tests_aux.h
    src/small_test_base/labels.c
    src/small_test_base/tests.c
)
