/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <runbmc/core/bmc.h>

LOG_MODULE_REGISTER(telemetry_engine, LOG_LEVEL_INF);

#define TELEMETRY_STACK_SIZE 2048
#define TELEMETRY_PRIORITY   9

K_THREAD_STACK_DEFINE(telemetry_stack, TELEMETRY_STACK_SIZE);
static struct k_thread telemetry_thread_data;

static void telemetry_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    struct bmc_telemetry_sample *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();

    LOG_INF("Telemetry engine thread started");

    while (1) {
        k_mutex_lock(mutex, K_FOREVER);

        state->sample_count++;
        state->uptime_sec = k_uptime_get_32() / 1000;

        k_mutex_unlock(mutex);

        k_sleep(K_MSEC(CONFIG_RUNBMC_TELEMETRY_INTERVAL_MS));
    }
}

int telemetry_engine_init(void)
{
    k_tid_t tid;

    tid = k_thread_create(&telemetry_thread_data, telemetry_stack,
                  K_THREAD_STACK_SIZEOF(telemetry_stack), telemetry_thread, NULL, NULL,
                  NULL, TELEMETRY_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "telemetry");

    return 0;
}
