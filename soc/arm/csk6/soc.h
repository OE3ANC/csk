#ifndef _CSK6_SOC_H_
#define _CSK6_SOC_H_


#include "ARMCM33_DSP_FP.h"
#include "cachel1_armv7.h"
#include <zephyr/devicetree.h>
void boot_cp(const void *run_address, const void *load_address, const uint32_t img_size);
#endif /* _CSK6_SOC_H_ */
