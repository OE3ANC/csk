
#define DT_DRV_COMPAT listenai_csk_gpio

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <sys/util.h>
#include "hal/gpio.h"
#include "hal/Driver_GPIO.h"


#define CSK_GPIO_PORT_MAX   2

typedef void *gpio_driver_ctx_t;

struct csk_gpio_config {
    int driver_num;
    void *arg;
};

static const gpio_driver_ctx_t csk_gpio_get_driver_ctx(int port)
{
    if (port == 0) {
        return GPIOA();
    } else if (port == 1) {
        return GPIOB();
    } else {
        return NULL;
    }
}

static void csk_gpio_isr(uint32_t event, void *arg)
{

}

static int csk_gpio_init(const struct device *dev)
{
    for (int port = 0; port < CSK_GPIO_PORT_MAX; port++) {
        gpio_driver_ctx_t gpio_driver_ctx = csk_gpio_get_driver_ctx(port);
        GPIO_Initialize(gpio_driver_ctx, csk_gpio_isr, (void *) gpio_driver_ctx); //init csk gpio driver
    }
    return 0;
}

static int csk_gpio_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);

    if (flags & GPIO_OUTPUT) {
        GPIO_SetDir(driver_context, 1UL << pin, CSK_GPIO_DIR_OUTPUT);
    } else if (flags & GPIO_INPUT) {
        GPIO_SetDir(driver_context, 1UL << pin, CSK_GPIO_DIR_INPUT);
    }

    GPIO_Control(driver_context, CSK_GPIO_MODE_PULL_NONE, 1UL << pin);
    if (flags & GPIO_PULL_UP) {
        GPIO_Control(driver_context, CSK_GPIO_MODE_PULL_UP, 1UL << pin);
    } else if (flags & GPIO_PULL_DOWN) {
        GPIO_Control(driver_context, CSK_GPIO_MODE_PULL_DOWN, 1UL << pin);
    }

    if (flags & GPIO_OUTPUT_INIT_HIGH) {
        GPIO_PinWrite(driver_context, 1UL << pin, 1);
    } else if (flags & GPIO_OUTPUT_INIT_LOW) {
        GPIO_PinWrite(driver_context, 1UL << pin, 0);
    }
    return 0;
}

static int csk_gpio_port_get_raw(const struct device *port, uint32_t *value)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);
    GPIO_RegDef *gpio_reg = ((GPIO_RESOURCES *)driver_context)->reg;
    *value = gpio_reg->REG_DATAIN.all;
    return 0;
}

int csk_gpio_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask, gpio_port_value_t value)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);
    GPIO_RegDef *gpio_reg = ((GPIO_RESOURCES *)driver_context)->reg;
    gpio_reg->REG_DATAOUT.all = (gpio_reg->REG_DATAOUT.all & ~mask) | (mask & value);
    return 0;
}

static int csk_gpio_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);
    GPIO_RegDef *gpio_reg = ((GPIO_RESOURCES *)driver_context)->reg;
    gpio_reg->REG_DATAOUT.all = pins;
    return 0;
}

static int csk_gpio_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);
    GPIO_RegDef *gpio_reg = ((GPIO_RESOURCES *)driver_context)->reg;
    gpio_reg->REG_DATAOUT.all &= ~pins;
    return 0;
}

static int csk_gpio_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
    const struct csk_gpio_config *config = port->config;
    gpio_driver_ctx_t driver_context = csk_gpio_get_driver_ctx(config->driver_num);
    GPIO_RegDef *gpio_reg = ((GPIO_RESOURCES *)driver_context)->reg;
    gpio_reg->REG_DATAOUT.all ^= pins;
    return 0;
}

static const struct gpio_driver_api csk_gpio_driver_api = {
        .pin_configure = csk_gpio_configure,
        .port_get_raw = csk_gpio_port_get_raw,
        .port_set_masked_raw = csk_gpio_port_set_masked_raw,
        .port_set_bits_raw = csk_gpio_port_set_bits_raw,
        .port_clear_bits_raw = csk_gpio_port_clear_bits_raw,
        .port_toggle_bits = csk_gpio_port_toggle_bits,
};


//TODO: Add obtaion configuration from devicetree MACRO
#define GPIO_CSK_INIT(n) \
    static const struct csk_gpio_config csk_gpio_##n##_config = {	\
		.driver_num = n,                           \
	};                      \
	DEVICE_DT_INST_DEFINE(n,					\
			    csk_gpio_init,			\
			    NULL,					\
			    NULL,                   \
                &csk_gpio_##n##_config,     \
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			    &csk_gpio_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(GPIO_CSK_INIT)

