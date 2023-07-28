#include "csk_sdk_version.h"
#include "zephyr/init.h"
#include "zephyr/kernel.h"

int version_init(void)
{
	printk("*** ListenAI CSK SDK Version: %s-%s ***\n", CSK_SDK_VERSION_STRING,
	       CSK_SDK_GIT_COMMIT_STRING);
	return 0;
}

SYS_INIT(version_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
