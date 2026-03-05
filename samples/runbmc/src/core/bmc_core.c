/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "runbmc/core/bmc.h"

#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
#include "runbmc/core/power.h"
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
#include "runbmc/core/sensors.h"
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
#include "runbmc/core/telemetry.h"
#endif

#ifdef CONFIG_RUNBMC_MCTP
#include "runbmc/protocols/mctp.h"
#endif

LOG_MODULE_REGISTER(bmc_core, LOG_LEVEL_INF);

/* Shared BMC state */
static struct bmc_state bmc_state;
static struct k_mutex bmc_mutex;

struct bmc_state *bmc_get_state_ptr(void)
{
    return &bmc_state;
}

struct k_mutex *bmc_get_mutex(void)
{
    return &bmc_mutex;
}

const struct bmc_state *bmc_get_status(void)
{
    return &bmc_state;
}

int bmc_core_init(void)
{
    int ret;

    LOG_INF("BMC Core initialization starting...");

    /* Initialize shared state and mutex */
    memset(&bmc_state, 0, sizeof(bmc_state));
    k_mutex_init(&bmc_mutex);

#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
    LOG_INF("Starting Power Management subsystem...");
    ret = power_mgmt_init();
    if (ret != 0) {
        LOG_ERR("Power Management init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
    LOG_INF("Starting Sensor Framework...");
    ret = sensor_framework_init();
    if (ret != 0) {
        LOG_ERR("Sensor Framework init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
    LOG_INF("Starting Telemetry Engine...");
    ret = telemetry_engine_init();
    if (ret != 0) {
        LOG_ERR("Telemetry Engine init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_MCTP
    LOG_INF("Starting MCTP Transport Layer...");

    /* Determine binding based on platform */
#if defined(CONFIG_RUNBMC_MCTP_I2C)
    uint8_t binding = MCTP_BINDING_I2C;
#elif defined(CONFIG_RUNBMC_MCTP_UART)
    uint8_t binding = MCTP_BINDING_UART;
#else
#error "No MCTP binding configured"
#endif

    /* Initialize MCTP with configured EID */
    ret = mctp_init(binding, CONFIG_RUNBMC_MCTP_EID);
    if (ret != 0) {
        LOG_ERR("MCTP init failed: %d", ret);
        return ret;
    }
#endif

    LOG_INF("BMC Core initialized successfully");
    return 0;
}
