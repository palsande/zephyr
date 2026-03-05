/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNBMC_CORE_TYPES_H_
#define RUNBMC_CORE_TYPES_H_

#include <stdint.h>

/* Maximum number of GPUs */
#ifndef CONFIG_RUNBMC_MAX_GPUS
#define CONFIG_RUNBMC_MAX_GPUS 8
#endif

/**
 * GPU Sensor Data
 */
struct gpu_sensor {
    int32_t temperature;  /* Temperature in degrees C */
    uint32_t power;       /* Power consumption in W */
    uint32_t utilization; /* GPU utilization 0-100% */
};

/**
 * BMC Telemetry Sample
 */
struct bmc_telemetry_sample {
    uint64_t timestamp;   /* System uptime in ms */
    uint32_t total_power; /* Total system power in W */
    struct gpu_sensor gpus[CONFIG_RUNBMC_MAX_GPUS];
};

/**
 * BMC State
 */
struct bmc_state {
    bool power_good;     /* System power is good */
    bool warning_active; /* Warning condition active */
    uint32_t uptime;     /* BMC uptime in seconds */
    struct bmc_telemetry_sample latest_sample;
};

#endif /* RUNBMC_CORE_TYPES_H_ */
