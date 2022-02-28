/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/ipm.h>
#include <avf_platform.h>



extern void csk6_ipm_callback_message(uint32_t id, volatile void *data);



/*between 4byte and 16byte*/
#define IPM_CSK_DATA_SIZE 16
#define IPM_CSK_DATA_REGS 4
static void ipm_callback(const struct device *dev, void *context,
			 uint32_t id, volatile void *data)
{
	ipm_callback_message(id, data);
}
static void ipm_init(void){

    struct device *ipm;
    uint32_t data32[IPM_CSK_DATA_SIZE / IPM_CSK_DATA_REGS] = {0};
    ipm = DEVICE_DT_GET(DT_NODELABEL(mailbox0));
	if (!device_is_ready(ipm)) {
		while (1) {
		}
	}
	ipm_register_callback(ipm, ipm_callback, NULL);
	ipm_set_enabled(ipm, 1);
    printk("IPM initialized, data size = %d\n",
		IPM_CSK_DATA_SIZE);
}



void ipm_callback_message(uint32_t id, volatile void *data){

    csk6_ipm_callback_message(id,data);
}




extern void example_sof_entry(void);

void sof_domain(void){

    ipm_init();
    avf_stream_platform_register();
    example_sof_entry();
}

