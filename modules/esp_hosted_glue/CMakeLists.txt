#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_ESP_HOSTED)

zephyr_library_named(esp_hosted_glue)

zephyr_include_directories(
    csk/app/include
    csk/common
    csk/port/include
    csk/driver/netif
    csk/driver/network
    csk/driver/serial
    csk/driver/transport
    csk/driver/transport/spi
    csk/driver/vhci
    csk/driver/hw_driver
    csk/os/include
)

zephyr_library_sources(
    csk/app/src/hosted.c
    csk/common/common.c
    csk/common/util.c
    csk/common/stats.c
    csk/port/src/platform_wrapper.c
    csk/driver/network/netdev_api.c
    csk/driver/network/netdev_stub.c
    csk/driver/serial/serial_ll_if.c
    csk/driver/transport/transport_drv.c
    csk/driver/transport/spi/spi_drv.c
    csk/driver/hw_driver/hosted_hw_drv.c
    csk/driver/vhci/vhci_if.c
    csk/os/src/csk_os_memory.c
    csk/os/src/csk_os_queue.c
    csk/os/src/csk_os_semaphore.c
    csk/os/src/csk_os_mutex.c
    csk/os/src/csk_os_thread.c
    csk/os/src/csk_os_timer.c
)
endif()
