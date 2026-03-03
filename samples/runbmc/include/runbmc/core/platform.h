/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#ifndef RUNBMC_CORE_PLATFORM_H_
#define RUNBMC_CORE_PLATFORM_H_

#include <stdint.h>

/**
 * @brief Platform-specific initialization
 * @return 0 on success, negative errno on failure
 */
int platform_init(void);

/**
 * @brief Platform-specific sensor read
 * @param sensor_id Sensor ID
 * @return Sensor value (scaled by 1000) or negative errno
 */
int32_t platform_read_sensor(uint8_t sensor_id);

/**
 * @brief Platform-specific power control
 * @param state Desired power state (0=off, 1=on)
 * @return 0 on success, negative errno on failure
 */
int platform_set_power(uint8_t state);

/**
 * @brief Get platform name
 * @return Platform name string
 */
const char *platform_get_name(void);

#endif /* RUNBMC_CORE_PLATFORM_H_ */
