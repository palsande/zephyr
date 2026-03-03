/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#ifndef RUNBMC_CORE_BMC_H_
#define RUNBMC_CORE_BMC_H_

#include <runbmc/core/types.h>

/**
 * @brief Initialize BMC core subsystem
 * @return 0 on success, negative errno on failure
 */
int bmc_core_init(void);

/**
 * @brief Get current BMC status
 * @param sample Pointer to telemetry sample structure
 * @return 0 on success, negative errno on failure
 */
int bmc_get_status(struct bmc_telemetry_sample *sample);

/**
 * @brief Get sensor reading
 * @param sensor_id Sensor ID
 * @param reading Pointer to sensor reading structure
 * @return 0 on success, negative errno on failure
 */
int bmc_get_sensor(uint8_t sensor_id, struct bmc_sensor_reading *reading);

/**
 * @brief Set power state
 * @param state Desired power state
 * @return 0 on success, negative errno on failure
 */
int bmc_set_power_state(enum bmc_power_state state);

/**
 * @brief Get global BMC state (thread-safe)
 * @return Pointer to global state structure
 */
struct bmc_telemetry_sample *bmc_get_state_ptr(void);

/**
 * @brief Get BMC state mutex
 * @return Pointer to mutex
 */
struct k_mutex *bmc_get_mutex(void);

#endif /* RUNBMC_CORE_BMC_H_ */
