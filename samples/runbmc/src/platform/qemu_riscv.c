/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <runbmc/core/platform.h>

LOG_MODULE_REGISTER(platform_qemu, LOG_LEVEL_INF);

int platform_init(void)
{
    LOG_INF("Initializing QEMU RISC-V platform");
    /* Platform-specific initialization */
    return 0;
}

int32_t platform_read_sensor(uint8_t sensor_id)
{
    /* Simulated sensor readings for QEMU */
    return 45000 + (k_uptime_get_32() % 10000); /* 45-55°C */
}

int platform_set_power(uint8_t state)
{
    LOG_INF("Platform power control: %s", state ? "ON" : "OFF");
    /* Simulated power control */
    return 0;
}

const char *platform_get_name(void)
{
#if defined(CONFIG_BOARD_QEMU_RISCV32)
    return "QEMU RISC-V32";
#elif defined(CONFIG_BOARD_QEMU_RISCV64)
    return "QEMU RISC-V64";
#else
    return "Unknown QEMU Platform";
#endif
}
