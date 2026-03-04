/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 RunBMC Project */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <runbmc/core/bmc.h>
#include <runbmc/core/platform.h>

/* Platform-specific LED indicators (if available) */
extern void platform_indicate_power_good(bool good) __attribute__((weak));
extern void platform_indicate_warning(bool active) __attribute__((weak));

LOG_MODULE_REGISTER(power_mgmt, LOG_LEVEL_INF);

#define POWER_STACK_SIZE     2048
#define POWER_PRIORITY       5
#define POWER_WARN_THRESHOLD 750 /* Watts */

K_THREAD_STACK_DEFINE(power_stack, POWER_STACK_SIZE);
static struct k_thread power_thread_data;

static void power_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    struct bmc_telemetry_sample *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();
    bool over_limit = false;

    LOG_INF("Power management thread started");

    /* Simulate power-on sequence */
    k_mutex_lock(mutex, K_FOREVER);
    state->power_state = BMC_POWER_STATE_TRANSITION;
    k_mutex_unlock(mutex);

    LOG_INF("Power sequence: Enabling voltage rails...");
    k_sleep(K_MSEC(500));

    LOG_INF("Power sequence: Waiting for POWER_GOOD...");
    k_sleep(K_MSEC(300));

    LOG_INF("Power sequence: Releasing CPU reset...");
    k_sleep(K_MSEC(200));

    k_mutex_lock(mutex, K_FOREVER);
    state->power_state = BMC_POWER_STATE_S0;
    k_mutex_unlock(mutex);

    LOG_INF("Power sequence: System is ON");

    /* Indicate power good on platform LEDs */
    if (platform_indicate_power_good) {
        platform_indicate_power_good(true);
    }

    /* Monitor power consumption (silently) */
    while (1) {
        k_mutex_lock(mutex, K_FOREVER);

        /* Simulate power consumption: 200-800W */
        state->power_watts = 200 + (k_uptime_get_32() % 600);

        /* Only log state changes (over limit / back to normal) */
        if (state->power_watts > POWER_WARN_THRESHOLD) {
            if (!over_limit) {
                over_limit = true;
                /* Turn on warning LED */
                if (platform_indicate_warning) {
                    platform_indicate_warning(true);
                }
            }
            /* Silently monitor while over limit */
        } else {
            if (over_limit) {
                // LOG_INF("Power back to normal: %u W", state->power_watts);
                over_limit = false;
                /* Turn on warning LED */
                if (platform_indicate_warning) {
                    platform_indicate_warning(false);
                }
            }
            /* Silently monitor while normal */
        }

        k_mutex_unlock(mutex);

        k_sleep(K_SECONDS(3));
    }
}

int power_mgmt_init(void)
{
    k_tid_t tid;

    tid = k_thread_create(&power_thread_data, power_stack, K_THREAD_STACK_SIZEOF(power_stack),
                  power_thread, NULL, NULL, NULL, POWER_PRIORITY, 0, K_NO_WAIT);

    k_thread_name_set(tid, "power_mgmt");

    return 0;
}
