/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNBMC_CORE_PLATFORM_H_
#define RUNBMC_CORE_PLATFORM_H_

#include <stdint.h>

/**
 * Get platform name
 *
 * @return Platform name string
 */
const char *platform_get_name(void);

/**
 * Initialize platform-specific hardware
 *
 * @return 0 on success, negative errno on failure
 */
int platform_init(void);

/**
 * Read sensor value
 *
 * @param sensor_id Sensor ID
 * @return Sensor value (temperature in degrees C)
 */
int32_t platform_read_sensor(uint8_t sensor_id);

/**
 * Control power state
 *
 * @param state Power state (0=OFF, 1=ON)
 * @return 0 on success, negative errno on failure
 */
int platform_set_power(uint8_t state);

/**
 * Indicate power good status (visual feedback)
 */
void platform_indicate_power_good(void);

/**
 * Indicate warning condition (visual feedback)
 */
void platform_indicate_warning(void);

/**
 * Indicate activity (heartbeat visual feedback)
 */
void platform_indicate_activity(void);

#endif /* RUNBMC_CORE_PLATFORM_H_ */
