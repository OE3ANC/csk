#include <stdlib.h>
#include <zephyr.h>
#include <sys/printk.h>

#include "tester_lisa.h"




void main(void)
{
	printk("lisa porting test on %s !\n", CONFIG_BOARD);

	// lisa_https_test();
	// lisa_os_test();
	lisa_websocket_test();

	while (1) {
		k_sleep(K_MSEC(5000));
		printk("lisa porting test on !\n");
	}
}
