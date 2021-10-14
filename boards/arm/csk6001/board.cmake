# SPDX-License-Identifier: Apache-2.0

board_runner_args(csk6001jlink "--device=VENUS" "--speed=4000")
board_runner_args(jlink "--device=VENUS")

include(${ZEPHYR_BASE}/boards/common/csk6001jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
