/*
 * QEMU RISC-V Platform HAL
 * Simulated sensor readings for development/testing
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <runbmc/core/platform.h>
#include <runbmc/core/types.h>

const char *platform_get_name(void)
{
    return "QEMU RISC-V32";
}

int platform_init(void)
{
    printk("QEMU RISC-V Platform HAL initialized\n");
    return 0;
}

int platform_set_power(uint8_t state)
{
    /* QEMU has no real power control - just log the request */
    printk("Power state change requested: %s\n", state ? "ON" : "OFF");
    return 0;
}

void platform_indicate_power_good(void)
{
    /* QEMU has no physical LEDs - no action needed */
    /* In a real implementation, this could control a GPIO or log to console */
}

void platform_indicate_warning(void)
{
    /* QEMU has no physical LEDs - no action needed */
    /* In a real implementation, this could flash LEDs or trigger alerts */
}

void platform_indicate_activity(void)
{
    /* QEMU has no physical LEDs - no action needed */
    /* In a real implementation, this could pulse an activity LED */
}

int32_t platform_read_sensor(uint8_t sensor_id)
{
    /* Simulated temperature reading for QEMU */
    return 45 + (sensor_id * 5);
}
