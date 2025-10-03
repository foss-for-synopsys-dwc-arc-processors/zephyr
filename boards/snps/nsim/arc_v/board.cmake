# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS nsim)

string(SUBSTRING "${BOARD_QUALIFIERS}" 1 -1 NSIM_BASE_FILENAME)
string(REPLACE "/" "_" NSIM_BASE_FILENAME "${NSIM_BASE_FILENAME}")

board_set_flasher_ifnset(arc-nsim)
board_set_debugger_ifnset(arc-nsim)

# Add lldbac runner support for both flashing and debugging
board_set_flasher_ifnset(lldbac)
board_set_debugger_ifnset(lldbac)

set(NSIM_PROPS "${NSIM_BASE_FILENAME}.props")
board_runner_args(arc-nsim "--props=${NSIM_PROPS}")
board_runner_args(lldbac "--nsim-props=${NSIM_PROPS}")

board_finalize_runner_args(arc-nsim)
board_finalize_runner_args(lldbac)
include(${ZEPHYR_BASE}/boards/common/mdb-nsim.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
