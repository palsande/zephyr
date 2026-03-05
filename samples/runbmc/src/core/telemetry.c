/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "runbmc/core/bmc.h"

LOG_MODULE_REGISTER(telemetry_engine, LOG_LEVEL_INF);

#define TELEMETRY_STACK_SIZE 2048
#define TELEMETRY_PRIORITY   7

static void telemetry_thread(void *, void *, void *);

K_THREAD_STACK_DEFINE(telemetry_stack, TELEMETRY_STACK_SIZE);
static struct k_thread telemetry_thread_data;
static k_tid_t telemetry_tid;

static void telemetry_thread(void *p1, void *p2, void *p3)
{
    struct bmc_state *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();

    LOG_INF("Telemetry engine thread started");

    while (1) {
        k_sleep(K_SECONDS(10));

        k_mutex_lock(mutex, K_FOREVER);

        /* Update timestamp */
        state->latest_sample.timestamp = k_uptime_get();

        /* Log telemetry summary */
        LOG_DBG("Telemetry: Power=%uW, Uptime=%us", state->latest_sample.total_power,
            state->uptime);

        k_mutex_unlock(mutex);
    }
}

int telemetry_engine_init(void)
{
    telemetry_tid = k_thread_create(&telemetry_thread_data, telemetry_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_stack), telemetry_thread,
                    NULL, NULL, NULL, TELEMETRY_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(telemetry_tid, "telemetry");

    return 0;
}
