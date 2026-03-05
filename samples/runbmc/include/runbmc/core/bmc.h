/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNBMC_CORE_BMC_H_
#define RUNBMC_CORE_BMC_H_

#include "runbmc/core/types.h"

/**
 * Initialize BMC core subsystems
 *
 * @return 0 on success, negative errno on failure
 */
int bmc_core_init(void);

/**
 * Get pointer to shared BMC state
 *
 * @return Pointer to BMC state structure
 */
struct bmc_state *bmc_get_state_ptr(void);

/**
 * Get pointer to BMC mutex
 *
 * @return Pointer to BMC mutex
 */
struct k_mutex *bmc_get_mutex(void);

/**
 * Get current BMC status
 *
 * @return Pointer to current BMC status
 */
const struct bmc_state *bmc_get_status(void);

#endif /* RUNBMC_CORE_BMC_H_ */
