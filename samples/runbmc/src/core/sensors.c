/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "runbmc/core/bmc.h"
#include "runbmc/core/platform.h"

LOG_MODULE_REGISTER(sensor_framework, LOG_LEVEL_INF);

#define SENSOR_STACK_SIZE 2048
#define SENSOR_PRIORITY   6

static void sensor_thread(void *, void *, void *);

K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
static struct k_thread sensor_thread_data;
static k_tid_t sensor_tid;

static void sensor_thread(void *p1, void *p2, void *p3)
{
    struct bmc_state *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();

    LOG_INF("Sensor framework thread started");

    while (1) {
        k_sleep(K_SECONDS(5));

        k_mutex_lock(mutex, K_FOREVER);

        /* Read sensors for each GPU */
        for (int i = 0; i < CONFIG_RUNBMC_MAX_GPUS; i++) {
            state->latest_sample.gpus[i].temperature = platform_read_sensor(i);
            state->latest_sample.gpus[i].power =
                80 + (k_uptime_get_32() % 120) + i * 10;
            state->latest_sample.gpus[i].utilization = 40 + (k_uptime_get_32() % 60);
        }

        state->latest_sample.timestamp = k_uptime_get();

        k_mutex_unlock(mutex);
    }
}

int sensor_framework_init(void)
{
    sensor_tid = k_thread_create(&sensor_thread_data, sensor_stack,
                     K_THREAD_STACK_SIZEOF(sensor_stack), sensor_thread, NULL, NULL,
                     NULL, SENSOR_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(sensor_tid, "sensors");

    return 0;
}
