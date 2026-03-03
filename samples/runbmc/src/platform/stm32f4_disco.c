/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <runbmc/core/platform.h>

LOG_MODULE_REGISTER(platform_stm32f4, LOG_LEVEL_INF);

/* STM32F4 Discovery LEDs */
#define LED0_NODE DT_ALIAS(led0) /* Green LED (PD12) */
#define LED1_NODE DT_ALIAS(led1) /* Orange LED (PD13) */
#define LED2_NODE DT_ALIAS(led2) /* Red LED (PD14) */
#define LED3_NODE DT_ALIAS(led3) /* Blue LED (PD15) */

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

/* LED mapping for BMC status:
 * Green  = Power Good / System ON
 * Orange = Activity / Sensor reads
 * Red    = Power Warning / Error
 * Blue   = Telemetry active
 */

static bool leds_initialized = false;

int platform_init(void)
{
    int ret;

    LOG_INF("Initializing STM32F4 Discovery platform");

    /* Initialize LEDs */
    if (!gpio_is_ready_dt(&led_green)) {
        LOG_ERR("LED0 (green) not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_orange)) {
        LOG_ERR("LED1 (orange) not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_red)) {
        LOG_ERR("LED2 (red) not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_blue)) {
        LOG_ERR("LED3 (blue) not ready");
        return -ENODEV;
    }

    /* Configure LED pins as outputs */
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure green LED: %d", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure orange LED: %d", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure red LED: %d", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure blue LED: %d", ret);
        return ret;
    }

    leds_initialized = true;

    /* Power-on LED sequence */
    LOG_INF("Running LED test sequence...");
    gpio_pin_set_dt(&led_green, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_orange, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_red, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_blue, 1);
    k_msleep(500);

    /* Turn off all LEDs */
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_orange, 0);
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_blue, 0);

    LOG_INF("STM32F4 Discovery platform initialized");
    return 0;
}

int32_t platform_read_sensor(uint8_t sensor_id)
{
    /* For STM32F4 Discovery, we could read:
     * - Internal temperature sensor via ADC
     * - Accelerometer via SPI (LIS3DSH)
     * For now, return simulated values
     */

    /* Blink orange LED on sensor read (activity indicator) */
    if (leds_initialized) {
        static uint32_t blink_counter = 0;
        if ((++blink_counter % 20) == 0) { /* Blink every 20 reads */
            gpio_pin_toggle_dt(&led_orange);
        }
    }

    /* Simulated sensor readings for now */
    return 45000 + (k_uptime_get_32() % 15000); /* 45-60°C */
}

int platform_set_power(uint8_t state)
{
    LOG_INF("Platform power control: %s", state ? "ON" : "OFF");

    if (!leds_initialized) {
        return -ENODEV;
    }

    if (state) {
        /* Power ON - green LED on */
        gpio_pin_set_dt(&led_green, 1);
        gpio_pin_set_dt(&led_red, 0);
    } else {
        /* Power OFF - green LED off */
        gpio_pin_set_dt(&led_green, 0);
    }

    return 0;
}

const char *platform_get_name(void)
{
    return "STM32F4 Discovery";
}

/* Platform-specific functions for LED control */

void platform_indicate_power_good(bool good)
{
    if (leds_initialized) {
        gpio_pin_set_dt(&led_green, good ? 1 : 0);
    }
}

void platform_indicate_warning(bool active)
{
    if (leds_initialized) {
        gpio_pin_set_dt(&led_red, active ? 1 : 0);
    }
}

void platform_indicate_activity(bool active)
{
    if (leds_initialized) {
        gpio_pin_set_dt(&led_blue, active ? 1 : 0);
    }
}
