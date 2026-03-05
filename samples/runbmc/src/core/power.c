/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "runbmc/core/bmc.h"
#include "runbmc/core/platform.h"

LOG_MODULE_REGISTER(power_mgmt, LOG_LEVEL_INF);

#define POWER_STACK_SIZE     2048
#define POWER_PRIORITY       5
#define POWER_WARN_THRESHOLD 750

static void power_thread(void *, void *, void *);

/* Define thread stack and data, but don't auto-start */
K_THREAD_STACK_DEFINE(power_stack, POWER_STACK_SIZE);
static struct k_thread power_thread_data;
static k_tid_t power_tid;

static void power_thread(void *p1, void *p2, void *p3)
{
    struct bmc_state *state = bmc_get_state_ptr();
    struct k_mutex *mutex = bmc_get_mutex();
    bool over_limit = false;

    LOG_INF("Power management thread started");

    /* Simulate power-on sequence */
    LOG_INF("Power sequence: Enabling voltage rails...");
    platform_set_power(1);
    k_msleep(500);

    LOG_INF("Power sequence: Waiting for POWER_GOOD...");
    k_msleep(300);

    LOG_INF("Power sequence: Releasing CPU reset...");
    k_msleep(200);

    LOG_INF("Power sequence: System is ON");
    platform_indicate_power_good();

    k_mutex_lock(mutex, K_FOREVER);
    state->power_good = true;
    k_mutex_unlock(mutex);

    while (1) {
        k_sleep(K_SECONDS(3));

        k_mutex_lock(mutex, K_FOREVER);
        state->uptime = k_uptime_get_32() / 1000;

        /* Simulate power monitoring */
        uint32_t power = 200 + (k_uptime_get_32() % 600);
        state->latest_sample.total_power = power;

        if (power > POWER_WARN_THRESHOLD) {
            if (!over_limit) {
#if !defined(CONFIG_BOARD_QEMU_RISCV32)
                LOG_WRN("Power consumption: %uW (OVER LIMIT!)", power);
#endif
                platform_indicate_warning();
                state->warning_active = true;
                over_limit = true;
            }
        } else {
            if (over_limit) {
#if !defined(CONFIG_BOARD_QEMU_RISCV32)
                LOG_INF("Power back to normal: %uW", power);
#endif
                platform_indicate_power_good();
                state->warning_active = false;
                over_limit = false;
            }
        }

        k_mutex_unlock(mutex);
    }
}

int power_mgmt_init(void)
{
    /* Create and start the thread now, not at boot */
    power_tid =
        k_thread_create(&power_thread_data, power_stack, K_THREAD_STACK_SIZEOF(power_stack),
                power_thread, NULL, NULL, NULL, POWER_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(power_tid, "power_mgmt");

    return 0;
}