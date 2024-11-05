/*
 * Copyright (c) 2023 Anhui(Shenzhen) Listenai Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>

__no_optimization static void trigger_fault_divide_zero(void)
{
	int a = 1;
	int b = 0;

	/* divide by zero */
	a = a / b;

	printk("a is %d\n", a);
}

__no_optimization static void trigger_fault_stack_overflow(void)
{
	int a[CONFIG_MAIN_STACK_SIZE];

	for(uint32_t i = 0; i < CONFIG_MAIN_STACK_SIZE; i++)
	{
		a[i] = i;
		printk("a[%d] is %d\n", i, a[i]);
	}

}

__no_optimization static void trigger_fault_null_pointer(void)
{
	int *p = NULL;

	*p = 1;
	printk("*(0x00) = %d\n", *p);
}

__no_optimization static void func1(void)
{
	trigger_fault_null_pointer();
	trigger_fault_stack_overflow();
	trigger_fault_divide_zero();
}

__no_optimization static void func2(void)
{
	int x[10];
	int y[10];
	for(uint32_t i = 1; i < 10; i++)
	{
		x[i] = i;
		y[i] = i;
		x[i] = x[i] / y[i];
	}

	func1();
}

__no_optimization static void func3(void)
{
	int x[10];
	int y[10];
	for(uint32_t i = 1; i < 10; i++)
	{
		x[i] = i;
		y[i] = i;
		x[i] = x[i] / y[i];
	}
	func2();
}

__no_optimization static void fault_thread(void *dummy1, void *dummy2, void *dummy3)			
{
	func3();
	while(1){
		k_msleep(1000);
	}
}	

K_THREAD_DEFINE(fault_thread_id, 4096,
                fault_thread, NULL, NULL, NULL,
                1, 0, 0);
				
__no_optimization int main(void)
{
	// func3();
	while(1){
		k_msleep(1000);
	}

	return -1;
}
