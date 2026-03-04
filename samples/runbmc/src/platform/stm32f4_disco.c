/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "runbmc/core/platform.h"

LOG_MODULE_REGISTER(platform_stm32f4, LOG_LEVEL_INF);

/* STM32F4 Discovery has 4 LEDs on PD12-PD15 */
#define LED_GREEN_NODE  DT_ALIAS(led0) /* PD12 - Green  */
#define LED_ORANGE_NODE DT_ALIAS(led1) /* PD13 - Orange */
#define LED_RED_NODE    DT_ALIAS(led2) /* PD14 - Red    */
#define LED_BLUE_NODE   DT_ALIAS(led3) /* PD15 - Blue   */

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);
static const struct gpio_dt_spec led_orange = GPIO_DT_SPEC_GET(LED_ORANGE_NODE, gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(LED_BLUE_NODE, gpios);

/* LED Assignment for Power Sequencing:
 * Green  (PD12) - Main power rail
 * Orange (PD13) - Secondary power rail
 * Red    (PD14) - CPU power rail
 * Blue   (PD15) - Peripheral power rail
 *
 * All ON = Power sequence complete, all rails good
 * Any OFF = Problem with that specific power rail
 */

const char *platform_get_name(void)
{
    return "STM32F4 Discovery";
}

int platform_init(void)
{
    LOG_INF("Initializing STM32F4 Discovery platform");

    /* Configure all LEDs */
    if (!gpio_is_ready_dt(&led_green) || !gpio_is_ready_dt(&led_orange) ||
        !gpio_is_ready_dt(&led_red) || !gpio_is_ready_dt(&led_blue)) {
        LOG_ERR("LED GPIO devices not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_orange, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);

    /* Run LED test sequence to verify all LEDs work */
    LOG_INF("Running LED test sequence...");

    /* Test each LED individually */
    gpio_pin_set_dt(&led_green, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_green, 0);

    gpio_pin_set_dt(&led_orange, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_orange, 0);

    gpio_pin_set_dt(&led_red, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_red, 0);

    gpio_pin_set_dt(&led_blue, 1);
    k_msleep(200);
    gpio_pin_set_dt(&led_blue, 0);

    /* All LEDs on briefly */
    gpio_pin_set_dt(&led_green, 1);
    gpio_pin_set_dt(&led_orange, 1);
    gpio_pin_set_dt(&led_red, 1);
    gpio_pin_set_dt(&led_blue, 1);
    k_msleep(300);

    /* All off - will turn on during power sequence */
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_orange, 0);
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_blue, 0);

    LOG_INF("STM32F4 Discovery platform initialized");
    return 0;
}

void platform_indicate_power_good(void)
{
    /* All LEDs ON = Power sequence complete, all rails good */
    LOG_INF("Power Good - Turning ON all LEDs (all power rails active)");
    gpio_pin_set_dt(&led_green, 1);  /* Main power */
    gpio_pin_set_dt(&led_orange, 1); /* Secondary power */
    gpio_pin_set_dt(&led_red, 1);    /* CPU power */
    gpio_pin_set_dt(&led_blue, 1);   /* Peripheral power */
}

void platform_indicate_warning(void)
{
    /* Flash all LEDs rapidly to indicate warning */
    gpio_pin_toggle_dt(&led_green);
    gpio_pin_toggle_dt(&led_orange);
    gpio_pin_toggle_dt(&led_red);
    gpio_pin_toggle_dt(&led_blue);
}

void platform_indicate_activity(void)
{
    /* Toggle blue LED for activity heartbeat */
    gpio_pin_toggle_dt(&led_blue);
}

int platform_set_power(uint8_t state)
{
    /* state: 0=OFF, 1=ON, other values for specific sequences */
    LOG_DBG("Set power state to %u", state);

    if (state == 0) {
        /* Power OFF - turn all LEDs off */
        gpio_pin_set_dt(&led_green, 0);
        gpio_pin_set_dt(&led_orange, 0);
        gpio_pin_set_dt(&led_red, 0);
        gpio_pin_set_dt(&led_blue, 0);
    } else if (state == 1) {
        /* Power ON - will be handled by platform_indicate_power_good() */
        /* This simulates enabling power rails in sequence */
    }

    return 0;
}

int32_t platform_read_sensor(uint8_t sensor_id)
{
    /* TODO: Read real sensors via I2C/ADC in Day 2 */
    /* For now, return simulated temperature values */
    return 45 + (sensor_id * 5); /* 45°C, 50°C, 55°C... */
}
