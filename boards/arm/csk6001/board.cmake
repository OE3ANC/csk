# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=Venus")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
