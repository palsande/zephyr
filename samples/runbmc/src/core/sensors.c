/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <runbmc/core/bmc.h>
#include <runbmc/core/platform.h>

LOG_MODULE_REGISTER(sensor_framework, LOG_LEVEL_INF);

#define SENSOR_STACK_SIZE 2048
#define SENSOR_PRIORITY   7

K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
static struct k_thread sensor_thread_data;

static void sensor_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    struct bmc_telemetry_sample *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();

    LOG_INF("Sensor framework thread started");

    while (1) {
        k_mutex_lock(mutex, K_FOREVER);

        /* Read CPU temperature */
        state->cpu_temp = 35 + (k_uptime_get_32() % 50);

        /* Read GPU temperatures */
        for (int i = 0; i < CONFIG_RUNBMC_MAX_GPUS; i++) {
            state->gpu_temp[i] = 40 + (k_uptime_get_32() % 60) + i * 2;
        }

        state->sensor_reads++;

        k_mutex_unlock(mutex);

        k_sleep(K_MSEC(CONFIG_RUNBMC_SENSOR_POLL_INTERVAL_MS));
    }
}

int sensor_framework_init(void)
{
    k_tid_t tid;

    tid = k_thread_create(&sensor_thread_data, sensor_stack,
                  K_THREAD_STACK_SIZEOF(sensor_stack), sensor_thread, NULL, NULL, NULL,
                  SENSOR_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "sensor_mon");

    return 0;
}
