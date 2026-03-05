/*
 * Copyright (c) 2026 RunBMC Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include "runbmc/core/bmc.h"
#include "runbmc/core/platform.h"

/* bmc info - Show BMC information */
static int cmd_bmc_info(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "RunBMC Firmware Information:");
    shell_print(sh, "  Version:  v%d.%d.%d", RUNBMC_VERSION_MAJOR, RUNBMC_VERSION_MINOR,
            RUNBMC_VERSION_PATCH);
    shell_print(sh, "  Platform: %s", platform_get_name());
    shell_print(sh, "  Zephyr:   v4.2.99");
    shell_print(sh, "  Build:    %s %s", __DATE__, __TIME__);

    return 0;
}

/* bmc status - Show BMC status */
static int cmd_bmc_status(const struct shell *sh, size_t argc, char **argv)
{
    const struct bmc_state *state = bmc_get_status();

    shell_print(sh, "BMC Status:");
    shell_print(sh, "  Power Good:   %s", state->power_good ? "Yes" : "No");
    shell_print(sh, "  Warning:      %s", state->warning_active ? "Active" : "None");
    shell_print(sh, "  Uptime:       %u seconds", state->uptime);
    shell_print(sh, "  Total Power:  %u W", state->latest_sample.total_power);

    return 0;
}

/* bmc features - Show enabled features */
static int cmd_bmc_features(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Enabled Features:");

#ifdef CONFIG_RUNBMC_POWER_MANAGEMENT
    shell_print(sh, "  [x] Power Management");
#else
    shell_print(sh, "  [ ] Power Management");
#endif

#ifdef CONFIG_RUNBMC_SENSOR_FRAMEWORK
    shell_print(sh, "  [x] Sensor Framework");
#else
    shell_print(sh, "  [ ] Sensor Framework");
#endif

#ifdef CONFIG_RUNBMC_TELEMETRY
    shell_print(sh, "  [x] Telemetry Collection");
#else
    shell_print(sh, "  [ ] Telemetry Collection");
#endif

#ifdef CONFIG_RUNBMC_MCTP
    shell_print(sh, "  [x] MCTP Transport");
#else
    shell_print(sh, "  [ ] MCTP Transport");
#endif

#ifdef CONFIG_RUNBMC_PLDM
    shell_print(sh, "  [x] PLDM Protocol");
#else
    shell_print(sh, "  [ ] PLDM Protocol");
#endif

    return 0;
}

/* bmc sensors - Show sensor data */
static int cmd_bmc_sensors(const struct shell *sh, size_t argc, char **argv)
{
    const struct bmc_state *state = bmc_get_status();

    shell_print(sh, "GPU Sensors (%u GPUs):", CONFIG_RUNBMC_MAX_GPUS);
    shell_print(sh, "  GPU  | Temp(C) | Power(W) | Util(%%)");
    shell_print(sh, "  -----|---------|----------|--------");

    for (int i = 0; i < CONFIG_RUNBMC_MAX_GPUS; i++) {
        shell_print(sh, "  %3d  |  %5d  |   %5u  |  %3u", i,
                state->latest_sample.gpus[i].temperature,
                state->latest_sample.gpus[i].power,
                state->latest_sample.gpus[i].utilization);
    }

    return 0;
}

/* bmc telemetry - Show telemetry data */
static int cmd_bmc_telemetry(const struct shell *sh, size_t argc, char **argv)
{
    const struct bmc_state *state = bmc_get_status();

    shell_print(sh, "Telemetry Data:");
    shell_print(sh, "  Timestamp:    %llu ms", state->latest_sample.timestamp);
    shell_print(sh, "  Total Power:  %u W", state->latest_sample.total_power);

    return 0;
}

/* Register BMC shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(bmc_cmds,
                   SHELL_CMD(info, NULL, "Show BMC information", cmd_bmc_info),
                   SHELL_CMD(status, NULL, "Show BMC status", cmd_bmc_status),
                   SHELL_CMD(features, NULL, "Show enabled features", cmd_bmc_features),
                   SHELL_CMD(sensors, NULL, "Show sensor data", cmd_bmc_sensors),
                   SHELL_CMD(telemetry, NULL, "Show telemetry data", cmd_bmc_telemetry),
                   SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bmc, &bmc_cmds, "BMC commands", NULL);
