# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# This CMake module will load all Zephyr CMake modules in correct order for
# default Zephyr build system.
#
# Outcome:
# See individual CMake module descriptions

include_guard(GLOBAL)

# The code line below defines the real minimum supported CMake version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to define the
# required version, not even invoking it from a CMake module is sufficient.
# It is however permitted to have multiple invocations of cmake_minimum_required.
cmake_minimum_required(VERSION 3.20.0)

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

find_package(ZephyrBuildConfiguration
  QUIET NO_POLICY_SCOPE
  NAMES ZephyrBuild
  PATHS ${ZEPHYR_BASE}/../*
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_PACKAGE_REGISTRY
  NO_CMAKE_SYSTEM_PATH
  NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
)

# Load Zephyr extensions
include(extensions)
include(git)
include(version)  # depends on hex.cmake

#
# Find tools
#

include(python)
include(west)
include(ccache)

# Load default root settings
include(root)

#
# Find Zephyr modules.
# Those may contain additional DTS, BOARD, SOC, ARCH ROOTs.
# Also create the Kconfig binary dir for generated Kconf files.
#
include(zephyr_module)

include(boards)
include(shields)
include(arch)
include(build_configuration)

include(user_cache)
include(verify-toolchain)
include(host-tools)

# Include board specific device-tree flags before parsing.
include(${BOARD_DIR}/pre_dt_board OPTIONAL)

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
#
# The DT system uses a C preprocessor for it's code generation needs.
# This creates an awkward chicken-and-egg problem, because we don't
# always know exactly which toolchain the user needs until we know
# more about the target, e.g. after DT and Kconfig.
#
# To resolve this we find "some" C toolchain, configure it generically
# with the minimal amount of configuration needed to have it
# preprocess DT sources, and then, after we have finished processing
# both DT and Kconfig we complete the target-specific configuration,
# and possibly change the toolchain.
include(generic_toolchain)
include(dts)
include(kconfig)
include(soc)
include(target_toolchain)
include(kernel)
