/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <runbmc/core/bmc.h>

LOG_MODULE_REGISTER(bmc_core, LOG_LEVEL_INF);

/* Global BMC state */
static struct bmc_telemetry_sample g_bmc_state;
static struct k_mutex g_bmc_mutex;

/* Function prototypes for optional subsystems */
#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
extern int power_mgmt_init(void);
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
extern int sensor_framework_init(void);
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
extern int telemetry_engine_init(void);
#endif

#ifdef CONFIG_RUNBMC_THERMAL_MANAGEMENT
extern int thermal_mgmt_init(void);
#endif

int bmc_core_init(void)
{
    int ret;

    LOG_INF("BMC Core initialization starting...");

    /* Initialize mutex */
    k_mutex_init(&g_bmc_mutex);

    /* Initialize state */
    memset(&g_bmc_state, 0, sizeof(g_bmc_state));
    g_bmc_state.power_state = BMC_POWER_STATE_UNKNOWN;

    /* Initialize subsystems based on Kconfig */
#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
    LOG_INF("Starting Power Management subsystem...");
    ret = power_mgmt_init();
    if (ret < 0) {
        LOG_ERR("Power Management init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
    LOG_INF("Starting Sensor Framework...");
    ret = sensor_framework_init();
    if (ret < 0) {
        LOG_ERR("Sensor Framework init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
    LOG_INF("Starting Telemetry Engine...");
    ret = telemetry_engine_init();
    if (ret < 0) {
        LOG_ERR("Telemetry Engine init failed: %d", ret);
        return ret;
    }
#endif

#ifdef CONFIG_RUNBMC_THERMAL_MANAGEMENT
    LOG_INF("Starting Thermal Management...");
    ret = thermal_mgmt_init();
    if (ret < 0) {
        LOG_ERR("Thermal Management init failed: %d", ret);
        return ret;
    }
#endif

    LOG_INF("BMC Core initialized successfully");
    return 0;
}

int bmc_get_status(struct bmc_telemetry_sample *sample)
{
    if (!sample) {
        return -EINVAL;
    }

    k_mutex_lock(&g_bmc_mutex, K_FOREVER);
    memcpy(sample, &g_bmc_state, sizeof(*sample));
    k_mutex_unlock(&g_bmc_mutex);

    return 0;
}

int bmc_get_sensor(uint8_t sensor_id, struct bmc_sensor_reading *reading)
{
    /* Placeholder - will be implemented in sensors.c */
    return -ENOSYS;
}

int bmc_set_power_state(enum bmc_power_state state)
{
    /* Placeholder - will be implemented in power.c */
    return -ENOSYS;
}

struct bmc_telemetry_sample *bmc_get_state_ptr(void)
{
    return &g_bmc_state;
}

struct k_mutex *bmc_get_mutex(void)
{
    return &g_bmc_mutex;
}
