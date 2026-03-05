/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "runbmc/core/bmc.h"
#include "runbmc/core/platform.h"

LOG_MODULE_REGISTER(runbmc_main, LOG_LEVEL_INF);

int main(void)
{
    /* Wait for console to be ready (especially needed for QEMU) */
    k_msleep(50);

    /* Print banner */
    printk("\n");
    printk("====================================================\n");
    printk("       RunBMC - BMC Firmware Framework on MCU\n");
    printk("            on Zephyr RTOS\n");
    printk("====================================================\n");
    printk("RunBMC:     v%d.%d.%d\n", RUNBMC_VERSION_MAJOR, RUNBMC_VERSION_MINOR,
           RUNBMC_VERSION_PATCH);
    printk("Zephyr:     v4.2.99\n");
    printk("Platform:   %s\n", platform_get_name());
    printk("Build:      %s %s\n", __DATE__, __TIME__);
    printk("====================================================\n");
    printk("Enabled Features:\n");

#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
    printk("  [x] Power Management\n");
#else
    printk("  [ ] Power Management\n");
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
    printk("  [x] Sensor Framework (%d GPUs)\n", CONFIG_RUNBMC_MAX_GPUS);
#else
    printk("  [ ] Sensor Framework\n");
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
    printk("  [x] Telemetry Collection\n");
#else
    printk("  [ ] Telemetry Collection\n");
#endif

#ifdef CONFIG_RUNBMC_THERMAL
    printk("  [x] Thermal Management\n");
#else
    printk("  [ ] Thermal Management\n");
#endif

#ifdef CONFIG_RUNBMC_PLDM
    printk("  [x] PLDM Protocol\n");
#else
    printk("  [ ] PLDM Protocol\n");
#endif

#ifdef CONFIG_RUNBMC_MCTP
    printk("  [x] MCTP Transport\n");
#else
    printk("  [ ] MCTP Transport\n");
#endif

#ifdef CONFIG_RUNBMC_FRU
    printk("  [x] FRU Management\n");
#else
    printk("  [ ] FRU Management\n");
#endif

    printk("====================================================\n\n");

    /* Small delay to let banner complete before threads start logging */
    k_msleep(10);

    /* Initialize platform */
    LOG_INF("Initializing platform...");
    int ret = platform_init();
    if (ret != 0) {
        LOG_ERR("Platform init failed: %d", ret);
        return ret;
    }

    /* Initialize BMC core (this starts all threads) */
    LOG_INF("Initializing BMC core...");
    ret = bmc_core_init();
    if (ret != 0) {
        LOG_ERR("BMC core init failed: %d", ret);
        return ret;
    }

    LOG_INF("RunBMC initialization complete");
    LOG_INF("Type 'bmc help' in shell for commands");

    return 0;
}
