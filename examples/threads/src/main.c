/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

#include<zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(LED1_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(LED2_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS_OKAY(LED3_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

K_THREAD_STACK_DEFINE(blink0_stk, STACKSIZE);
K_THREAD_STACK_DEFINE(blink1_stk, STACKSIZE);
K_THREAD_STACK_DEFINE(blink2_stk, STACKSIZE);
K_THREAD_STACK_DEFINE(blink3_stk, STACKSIZE);
K_THREAD_STACK_DEFINE(uart_out_stk, STACKSIZE);

K_FIFO_DEFINE(printk_fifo);

static struct k_thread blink0_thrd, blink1_thrd, blink2_thrd, blink3_thrd, uart_out_thrd;

struct printk_data_t {
    void *fifo_reserved; /* 1st word reserved for use by fifo */
    uint32_t led;
    uint32_t cnt;
};

struct led {
    struct gpio_dt_spec spec;
    uint8_t num;
};

static const struct led led0 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
    .num = 0,
};

static const struct led led1 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
    .num = 1,
};

static const struct led led2 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED2_NODE, gpios, {0}),
    .num = 2,
};

static const struct led led3 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED3_NODE, gpios, {0}),
    .num = 3,
};

void blink(const struct led *led, uint32_t sleep_ms, uint32_t id)
{
    const struct gpio_dt_spec *spec = &led->spec;
    int cnt = 0;
    int ret;

    if (!device_is_ready(spec->port)) {
        printk("Error: %s device is not ready\n", spec->port->name);
        return;
    }

    ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure pin %d (LED '%d')\n",
            ret, spec->pin, led->num);
        return;
    }

    while (1) {
        //printk("blink%d\n", id);
        gpio_pin_set(spec->port, spec->pin, cnt % 2);

        struct printk_data_t tx_data = { .led = id, .cnt = cnt };

        size_t size = sizeof(struct printk_data_t);
        char *mem_ptr = k_malloc(size);
        __ASSERT_NO_MSG(mem_ptr != 0);

        memcpy(mem_ptr, &tx_data, size);

        k_fifo_put(&printk_fifo, mem_ptr);

        k_msleep(sleep_ms);
        cnt++;
    }
}

void blink0(void)
{
    blink(&led0, 100, 0);
}

void blink1(void)
{
    blink(&led1, 1000, 1);
}

void blink2(void)
{
    blink(&led2, 10000, 2);
}

void blink3(void)
{
    blink(&led3, 100000, 3);
}

void uart_out(void)
{
    while (1) {
        //printk("uart_out()\n");
        struct printk_data_t *rx_data = k_fifo_get(&printk_fifo,
                               K_FOREVER);
        if(rx_data->led == 2)
            printk("Toggled led%d; counter=%d\n",
               rx_data->led, rx_data->cnt);
        else if(rx_data->led == 3)
            printk("Toggled led%d; counter=%d\n",
               rx_data->led, rx_data->cnt);
        k_free(rx_data);
    }
}

int main(void)
{
    printk("main()\n");

    k_thread_create(&blink0_thrd, blink0_stk, STACKSIZE, (k_thread_entry_t)blink0, NULL, NULL, NULL,
    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&blink1_thrd, blink1_stk, STACKSIZE, (k_thread_entry_t)blink1, NULL, NULL, NULL,
    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&blink2_thrd, blink2_stk, STACKSIZE, (k_thread_entry_t)blink2, NULL, NULL, NULL,
    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&blink3_thrd, blink3_stk, STACKSIZE, (k_thread_entry_t)blink3, NULL, NULL, NULL,
    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&uart_out_thrd, uart_out_stk, STACKSIZE, (k_thread_entry_t)uart_out, NULL, NULL, NULL,
    PRIORITY, 0, K_NO_WAIT);

    printk("main()\n");

    return 0;
}

#if 0
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL,
        PRIORITY, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1, NULL, NULL, NULL,
        PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL,
        PRIORITY, 0, 0);
#endif