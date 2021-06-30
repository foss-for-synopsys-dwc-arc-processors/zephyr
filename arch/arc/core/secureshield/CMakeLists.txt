#
# Copyright (c) 2018 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(
  CONFIG_ARC_SECURE_FIRMWARE
  arc_sjli.c
  arc_secure.S
  secure_sys_services.c
  )

zephyr_library_sources_ifdef(
  CONFIG_ARC_NORMAL_FIRMWARE
  arc_normal.c
  )
