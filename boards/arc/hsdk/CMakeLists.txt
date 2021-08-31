#
# Copyright (c) 2019 Synopsys, Inc. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_PINMUX_HSDK pinmux.c)
zephyr_sources_ifdef(CONFIG_ARC_MPU_ENABLE arc_mpu_regions.c)
