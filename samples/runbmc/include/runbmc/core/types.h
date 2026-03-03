/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#ifndef RUNBMC_CORE_TYPES_H_
#define RUNBMC_CORE_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/* Power states (ACPI-like) */
enum bmc_power_state {
    BMC_POWER_STATE_S5 = 0,         /* Soft Off */
    BMC_POWER_STATE_S0 = 1,         /* On */
    BMC_POWER_STATE_TRANSITION = 2, /* Transitioning */
    BMC_POWER_STATE_UNKNOWN = 3,
};

/* Sensor types */
enum bmc_sensor_type {
    BMC_SENSOR_TYPE_TEMPERATURE = 0,
    BMC_SENSOR_TYPE_VOLTAGE = 1,
    BMC_SENSOR_TYPE_CURRENT = 2,
    BMC_SENSOR_TYPE_POWER = 3,
    BMC_SENSOR_TYPE_FAN = 4,
};

/* Sensor reading structure */
struct bmc_sensor_reading {
    uint8_t sensor_id;
    enum bmc_sensor_type type;
    int32_t value;      /* Scaled by 1000 (e.g., 45.5°C = 45500) */
    uint32_t timestamp; /* Milliseconds since boot */
    bool valid;
};

/* Telemetry sample */
struct bmc_telemetry_sample {
    uint32_t uptime_sec;
    enum bmc_power_state power_state;
    int16_t cpu_temp;
    int16_t gpu_temp[CONFIG_RUNBMC_MAX_GPUS];
    uint16_t power_watts;
    uint32_t sensor_reads;
    uint32_t sample_count;
};

#endif /* RUNBMC_CORE_TYPES_H_ */
