/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNBMC_CORE_POWER_H_
#define RUNBMC_CORE_POWER_H_

/**
 * Initialize Power Management subsystem
 *
 * @return 0 on success, negative errno on failure
 */
int power_mgmt_init(void);

#endif /* RUNBMC_CORE_POWER_H_ */
